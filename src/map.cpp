#include <cstring>
#include <cstdlib>
//
#define GLM_FORCE_PURE
#include <glm/gtc/noise.hpp>
//
#include <lux_shared/common.hpp>
#include <lux_shared/map.hpp>
#include <lux_shared/noise.hpp>
#include <lux_shared/uninit_obj.hpp>
#include <lux_shared/marching_cubes.hpp>
//
#include <db.hpp>
#include <entity.hpp>
#include <physics.hpp>
#include "map.hpp"

struct SuspendedChunk {
    BitArr<CHK_VOL> mask;
    Arr<Block, CHK_VOL> blocks;
};

//@TODO move the meshes to physics.hpp?
//@TODO share mesh with client?
struct PhysicsMesh {
    struct TriangleData {
        DynArr<Vec3F> verts;
        DynArr<I32>   idxs;
    };
    struct BulletData {
        UninitObj<btTriangleIndexVertexArray> trigs_array;
        UninitObj<btBvhTriangleMeshShape>     shape;
    };

    static List<TriangleData> triangle_data;
    static List<BulletData>   bullet_data;

    btRigidBody* body;
    decltype(bullet_data)::iterator   bullet_iter;
    decltype(triangle_data)::iterator triangle_iter;
    bool has_body  = false;
    bool has_shape = false;
    PhysicsMesh() {
        triangle_data.emplace_back();
        triangle_iter = triangle_data.end();
        triangle_iter--;

        bullet_data.emplace_back();
        bullet_iter = bullet_data.end();
        bullet_iter--;

        has_body  = false;
        has_shape = false;
    }
    ~PhysicsMesh() {
        if(has_body) {
            physics_remove_body(body);
            bullet_iter->trigs_array.deinit();
        }
        if(has_shape) {
            bullet_iter->shape.deinit();
        }

        bullet_data.erase(bullet_iter);
        triangle_data.erase(triangle_iter);
    }
};

List<PhysicsMesh::TriangleData> PhysicsMesh::triangle_data;
List<PhysicsMesh::BulletData>   PhysicsMesh::bullet_data;

//@TODO might want to use the excess values somehow
typedef Arr<F32, (CHK_SIZE + 1) * (CHK_SIZE + 1)> HeightChunk;

static VecMap<ChkPos, Chunk> chunks;
static VecMap<ChkPos, SuspendedChunk> suspended_chunks;
static VecMap<ChkPos, PhysicsMesh> physics_meshes;
static VecMap<Vec2<ChkCoord>, HeightChunk> height_map;

F32 day_cycle;
VecSet<ChkPos> updated_chunks;
VecSet<ChkPos> light_updated_chunks;

constexpr Uns LIGHT_BITS_PER_COLOR = 4;
static_assert(LIGHT_BITS_PER_COLOR * 2 <= sizeof(LightLvl) * 8);
constexpr Uns LIGHT_RANGE          = 1 << LIGHT_BITS_PER_COLOR;

static void update_chunk_light(ChkPos const &pos, Chunk& chunk);
static bool is_chunk_loaded(ChkPos const& pos) {
    return chunks.count(pos) > 0;
}

static HeightChunk const& guarantee_height_chunk(Vec2<ChkCoord> const& pos) {
    constexpr Uns octaves    = 16;
    constexpr F32 base_scale = 0.001f;
    constexpr F32 h_exp      = 3.f;
    constexpr F32 max_h      = 512.f;
    if(height_map.count(pos) == 0) {
        auto& h_chunk = height_map[pos];
        Vec2F base_seed = (Vec2F)(pos * (ChkCoord)CHK_SIZE) * base_scale;
        Vec2F seed = base_seed;
        Uns idx = 0;
        for(Uns y = 0; y < CHK_SIZE + 1; ++y) {
            for(Uns x = 0; x < CHK_SIZE + 1; ++x) {
                h_chunk[idx] =
                    pow(u_norm(noise_fbm(seed, octaves)), h_exp) * max_h;
                seed.x += base_scale;
                ++idx;
            }
            seed.x  = base_seed.x;
            seed.y += base_scale;
        }
    }
    auto const& h_chunk = height_map[pos];
    return h_chunk;
}

static void write_suspended_block(MapPos const& pos, Block block) {
    ChkPos chk_pos = to_chk_pos(pos);
    ChkIdx chk_idx = to_chk_idx(pos);
    if(is_chunk_loaded(chk_pos)) {
        updated_chunks.insert(chk_pos);
        Chunk& chk = chunks.at(chk_pos);
        chk.blocks[chk_idx] = block;
        chk.updated_blocks.insert(chk_idx);
    } else {
        auto& chk = suspended_chunks[chk_pos];
        chk.blocks[chk_idx] = block;
        chk.mask[chk_idx] = true;
    }
}

static Chunk& load_chunk(ChkPos const& pos) {
    LUX_ASSERT(!is_chunk_loaded(pos));
    LUX_LOG("loading chunk");
    LUX_LOG("    pos: {%zd, %zd, %zd}", pos.x, pos.y, pos.z);
    ///@RESEARCH to do a better way to no-copy default construct
    Chunk& chunk = chunks[pos];
    static const BlockId raw_stone  = db_block_id("raw_stone"_l);
    static const BlockId wall       = db_block_id("stone_wall"_l);
    static const BlockId grass      = db_block_id("grass"_l);
    static const BlockId dark_grass = db_block_id("dark_grass"_l);
    static const BlockId dirt       = db_block_id("dirt"_l);
    static const BlockId leaves     = db_block_id("tree_leaves"_l);
    static const BlockId wood       = db_block_id("tree_trunk"_l);
    Vec2<ChkCoord> h_pos = pos;
    auto const& h_chunk = guarantee_height_chunk(h_pos);
    for(Uns i = 0; i < CHK_VOL; ++i) {
        MapPos map_pos = to_map_pos(pos, i);
        IdxPos idx_pos = to_idx_pos(i);
        auto get_h = [&](IdxPos p) {
            return h_chunk[p.x + p.y * (CHK_SIZE + 1)];
        };
        F32 h = get_h(idx_pos);
        MapCoord f_h = ceil(h);

        BlockId block;
        if(map_pos.z > f_h) {
            block = void_block;
        } else {
            F32 diffx =  abs(h - get_h(idx_pos + IdxPos(1, 0, 0)));
            F32 diffy =  abs(h - get_h(idx_pos + IdxPos(0, 1, 0)));
            F32 diffxy = abs(h - get_h(idx_pos + IdxPos(1, 1, 0))) / sqrt(2.f);
            F32 diff = (diffx + diffy + diffxy) / 3.f;
            if(map_pos.z >= f_h - 2 - diff * 10.f) {
                if(diff < 0.7f) {
                    block = dark_grass;
                } else if(diff < 1.f) {
                    block = dirt;
                } else {
                    block = raw_stone;
                }
            } else if(map_pos.z >= f_h - 4) {
                block = dirt;
            } else {
                block = raw_stone;
            }
        }
        F32 r_h = clamp(h - (F32)map_pos.z, 0.f, 1.f);
        chunk.blocks[i].id  = block;
        chunk.blocks[i].lvl = (r_h * 15.f);
        /*if(chunk.blocks[i] == void_block) {
            chunk.light_lvl[i] = 0x0F00;
        } else {
            chunk.light_lvl[i] = 0x0000;
        }
        if(chunk.blocks[i] == void_block && lux_randf(map_pos) > 0.999f) {
            add_light_node(to_map_pos(pos, i), 0.5f);
        }*/
    }
#if 1
    for(Uns i = 0; i < CHK_VOL; ++i) {
        MapCoord h = round(h_chunk[i & ((CHK_SIZE * CHK_SIZE) - 1)]);
        F32 f_h = ceil(h);
        MapPos map_pos = to_map_pos(pos, i);
        if(map_pos.z == f_h && lux_randf(map_pos) > .99f && chunk.blocks[i].id == dark_grass) {
            I32 h = lux_randmm(8, 20, map_pos, 0);
            for(Uns j = 0; j < h; ++j) {
                write_suspended_block(map_pos + MapPos(0, 0, j), {dirt, 0xff});
            }
            for(MapCoord z = -(h / 2 + 2); z <= h / 2; ++z) {
                for(MapCoord y = -5; y <= 5; ++y) {
                    for(MapCoord x = -5; x <= 5; ++x) {
                        F32 diff = (F32)((h / 2) + 2 - z) / 4.f - length(Vec2F(x, y));
                        F32 rand_factor = lux_randf(map_pos, 3, Vec3F(x, y, z));
                        if(diff > 0.f &&
                           lux_randf(map_pos, 1, Vec3F(x, y, z)) <
                           (((F32)z + (h / 2 + 2)) / ((F32)h * 2.f) + 0.5f) +
                           rand_factor) {
                            diff = min(1.f, diff);
                            write_suspended_block(map_pos + MapPos(x, y, h + z), {grass, diff * 255.f});
                        }
                    }
                }
            }
        }
    }
#endif
#if 1
    MapPos base_pos = to_map_pos(pos, 0);
    if(pos.z < 0) {
        U32 worms_num = lux_randf(pos) > 0.99 ? 1 : 0;
        for(Uns i = 0; i < worms_num; ++i) {
            Vec3F dir;
            U32 len = lux_randmm(10, 500, pos, i, 0);
            dir = lux_rand_norm(pos, i, 1);
            Vec3F map_pos = to_map_pos(pos, lux_randm(CHK_VOL, pos, i, 2));
            F32 rad = lux_randfmm(2, 5, pos, i, 3);
            for(Uns j = 0; j < len; ++j) {
                rad = noise_fbm(((F32)(j + i) * 1000.f +
                    length((Vec3F)base_pos)) * 0.05f, 3);
                rad = u_norm(rad) * 9.f + 1.f;
                rad *= 1.f - abs(s_norm((F32)j / (F32)len));
                Vec2<ChkCoord> h_pos = to_chk_pos(map_pos);
                HeightChunk const& h_chk = guarantee_height_chunk(h_pos);
                IdxPos idx_pos = to_idx_pos(map_pos);
                F32 h = h_chk[idx_pos.x + idx_pos.y * (CHK_SIZE + 1)];
                if(map_pos.z > h + rad) {
                    ///we have drilled too far into the surface by now
                    break;
                }
                MapCoord r_rad = ceil(rad);
                for(MapCoord z = -r_rad; z <= r_rad; ++z) {
                    for(MapCoord y = -r_rad; y <= r_rad; ++y) {
                        for(MapCoord x = -r_rad; x <= r_rad; ++x) {
                            if(length(Vec3F(x, y, z)) < rad) {
                                write_suspended_block(floor(map_pos + Vec3F(x, y, z)),
                                    {void_block, 0x00});
                            }
                        }
                    }
                }
                map_pos += dir;
                Vec3F ch = {lux_randf(pos, i, 5, j, 0) - 0.5f,
                            lux_randf(pos, i, 5, j, 1) - 0.5f,
                            lux_randf(pos, i, 5, j, 2) - 0.5f};
                ch *= 0.4f;
                dir += ch;
                dir = normalize(dir);
            }
        }
    }
#endif
    return chunk;
}

void guarantee_chunk(ChkPos const& pos) {
    if(!is_chunk_loaded(pos)) {
        load_chunk(pos);
    }
}

static void build_physics_mesh(ChkPos const& pos) {
    LUX_LOG_DBG("building physics_mesh {%zd, %zd, %zd}", pos.x, pos.y, pos.z);

    U32 trigs_num = 0;
    static DynArr<I32>    idxs(CHK_VOL * 5 * 3);
    static DynArr<Vec3F> verts(CHK_VOL * 5 * 3);

    MapPos constexpr axis_off[6] =
        {{-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}};

    constexpr MapPos cell_verts[8] = {
        {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
        {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
    GridCell grid_cell;
    for(Uns i = 0; i < 8; ++i) {
        grid_cell.p[i] = (Vec3F)cell_verts[i];
    }
    for(ChkIdx i = 0; i < CHK_VOL; ++i) {
        MapPos map_pos = to_map_pos(pos, i);
        Vec3F rel_pos = (Vec3F)to_idx_pos(i) + 0.5f;
        int sign = s_norm((F32)get_block(map_pos).lvl / 15.f) >= 0.f;
        bool has_face = false;
        for(Uns j = 0; j < 8; ++j) {
            MapPos abs_pos = map_pos + cell_verts[j];
            BlockLvl lvl = get_block(abs_pos).lvl;
            grid_cell.val[j] = s_norm((F32)lvl / 15.f);
            if((grid_cell.val[j] > 0.f) != sign) {
                has_face = true;
            }
        }
        if(!has_face) continue;
        Triangle cell_trigs[5];
        Uns cell_trigs_num = polygonise(grid_cell, 0.f, cell_trigs);
        for(Uns j = 0; j < cell_trigs_num; ++j) {
            for(Uns k = 0; k < 3; ++k) {
                idxs[ trigs_num * 3 + k] = trigs_num * 3 + k;
                verts[trigs_num * 3 + k] = cell_trigs[j].p[k] + rel_pos;
            }
            ++trigs_num;
        }
    }

    auto& mesh = physics_meshes[pos];

    if(mesh.has_body) {
        physics_remove_body(mesh.body);
        mesh.bullet_iter->trigs_array.deinit();
        mesh.has_body = false;
    }
    if(trigs_num > 0) {
        mesh.triangle_iter->verts.resize(trigs_num * 3);
        mesh.triangle_iter->verts.cpy(verts, trigs_num * 3);
        mesh.triangle_iter->idxs.resize(trigs_num * 3);
        mesh.triangle_iter->idxs.cpy(idxs, trigs_num * 3);

        mesh.bullet_iter->trigs_array.init(
            trigs_num    , (I32*)mesh.triangle_iter->idxs.beg , sizeof(I32) * 3,
            trigs_num * 3, (F32*)mesh.triangle_iter->verts.beg, sizeof(Vec3F));

        if(not mesh.has_shape) {
            mesh.bullet_iter->shape.init(
                &*mesh.bullet_iter->trigs_array, true, btVector3{0, 0, 0},
                btVector3{CHK_SIZE, CHK_SIZE, CHK_SIZE});
            mesh.has_shape = true;
        } else {
            /*mesh.bullet_iter->shape->getOptimizedBvh()->build(
                &*mesh.bullet_iter->trigs_array, true, btVector3{0, 0, 0},
                btVector3{CHK_SIZE, CHK_SIZE, CHK_SIZE});*/
            mesh.bullet_iter->shape->getOptimizedBvh()->refit(
                &*mesh.bullet_iter->trigs_array, btVector3{0, 0, 0},
                btVector3{CHK_SIZE, CHK_SIZE, CHK_SIZE});
            /*mesh.bullet_iter->shape->refitTree(btVector3{0, 0, 0},
                btVector3{CHK_SIZE, CHK_SIZE, CHK_SIZE});*/
        }

        mesh.body = physics_create_mesh(to_map_pos(pos, 0),
            &*mesh.bullet_iter->shape);
        mesh.has_body = true;
        //mesh.triangle_iter->idxs.clear();
        //mesh.triangle_iter->verts.clear();
    }
}

void guarantee_physics_mesh_around(ChkPos const& pos) {
    if(physics_meshes.count(pos) > 0) return;

    for(ChkCoord z = -2; z <= 2; ++z) {
        for(ChkCoord y = -2; y <= 2; ++y) {
            for(ChkCoord x = -2; x <= 2; ++x) {
                guarantee_chunk(ChkPos(x, y, z) + pos);
            }
        }
    }
    for(auto const& off : chebyshev<ChkCoord>) {
        if(physics_meshes.count(off + pos) > 0) continue;
        build_physics_mesh(off + pos);
    }
}

Chunk const& get_chunk(ChkPos const& pos) {
    LUX_ASSERT(is_chunk_loaded(pos));
    ///wish there was a non-bound-checking way to do it
    return chunks.at(pos);
}

Block& write_block(MapPos const& pos) {
    ChkPos chk_pos = to_chk_pos(pos);
    LUX_ASSERT(is_chunk_loaded(chk_pos));
    updated_chunks.emplace(chk_pos);
    Chunk& chunk = chunks.at(chk_pos);
    ChkIdx chk_idx = to_chk_idx(pos);
    chunk.updated_blocks.insert(chk_idx);
    return chunk.blocks[chk_idx];
}

Block get_block(MapPos const& pos) {
    return get_chunk(to_chk_pos(pos)).blocks[to_chk_idx(pos)];
}

BlockBp const& get_block_bp(MapPos const& pos) {
    return db_block_bp(get_block(pos).id);
}

//@CONSIDER separate file for lightsim
struct LightNode {
    U8       lum;
    ChkIdx   idx;
    static_assert(sizeof(lum) * 8 >= LIGHT_BITS_PER_COLOR);
};

VecMap<ChkPos, Queue<LightNode>> light_nodes;
VecSet<ChkPos>                   awaiting_light_updates;

void map_apply_suspended_updates() {
    for(auto it = suspended_chunks.begin(), end = suspended_chunks.end();
             it != end;) {
        if(is_chunk_loaded(it->first)) {
            auto const& src = it->second;
            updated_chunks.insert(it->first);
            Chunk& dst = chunks.at(it->first);
            for(Uns i = 0; i < CHK_VOL; ++i) {
                if(src.mask[i]) {
                    dst.blocks[i] = src.blocks[i];
                    dst.updated_blocks.insert(i);
                }
            }
            it = suspended_chunks.erase(it);
        } else ++it;
    }
}

void map_tick() {
    static Uns tick_num = 0;
    physics_tick(1.f / 64.f); //TODO tick time
    map_apply_suspended_updates();
#if 0
    static VecSet<MapPos> bob;
    static VecSet<MapPos> new_blocks;
    for(auto const& block : new_blocks) {
        bob.insert(block);
    }
    new_blocks.clear();
    for(auto const& pos : updated_chunks) {
        for(auto const& idx : get_chunk(pos).updated_blocks) {
            bob.insert(to_map_pos(pos, idx));
        }
    }
    const static BlockId stone_id = db_block_id("stone_wall"_l);
    for(auto it = bob.begin(), end = bob.end(); it != end; it++) {
        auto const& pos = *it;
        for(auto const& off : manhattan_hollow_2d<MapCoord>) {
            Block b = get_block(pos);
            MapPos off_pos = pos + MapPos(off, 0);
            Block const& r_blk = get_block(off_pos);
            Int diff = (Int)b.lvl - (Int)r_blk.lvl;
            if(diff >= 2) {
                Block& src_blk = write_block(pos);
                Block& dst_blk = write_block(off_pos);
                dst_blk.lvl++;
                src_blk.lvl--;
                dst_blk.id = src_blk.id;
                new_blocks.insert(off_pos);
                new_blocks.insert(pos);
            }
        }
    }
#endif
    for(auto const& update : awaiting_light_updates) {
        if(is_chunk_loaded(update)) {
            light_updated_chunks.emplace(update);
        }
    }
    for(auto const& update : light_updated_chunks) {
        update_chunk_light(update, chunks.at(update));
        awaiting_light_updates.erase(update);
    }
    for(auto const& pos : updated_chunks) {
        if(physics_meshes.count(pos) > 0) {
            U8 updated_sides = 0b000000;
            LUX_ASSERT(is_chunk_loaded(pos));
            Chunk const& chunk = get_chunk(pos);
            for(auto const& idx : chunk.updated_blocks) {
                IdxPos idx_pos = to_idx_pos(idx);
                for(Uns a = 0; a < 3; ++a) {
                    if(idx_pos[a] == 0) {
                        updated_sides |= (0b000001 << (a * 2));
                    } else if(idx_pos[a] == CHK_SIZE - 1) {
                        updated_sides |= (0b000010 << (a * 2));
                    }
                }
            }
            auto update_mesh = [&](ChkPos const& r_pos) {
                ChkPos off_pos = r_pos + pos;
                if(physics_meshes.count(off_pos) > 0) {
                    build_physics_mesh(off_pos);
                }
            };
            update_chunks_around(update_mesh, updated_sides);
        }
    }
    /*for(auto const& pair : chunks) {
        if(lux_randf(pair.first, 0, tick_num) > 0.999f) {
            ChkIdx idx = lux_randm(CHK_VOL, pair.first, 1, tick_num);
            Block& block = write_block(to_map_pos(pair.first, idx));
            block.id = 1;
            block.lvl = 0xff;
        }
    }*/
    Uns constexpr ticks_per_day = 1 << 12;
    day_cycle = 1.f;
    /*day_cycle = std::sin(tau *
        (((F32)(tick_num % ticks_per_day) / (F32)ticks_per_day) + 0.25f));*/
    tick_num++;
}

void add_light_node(MapPos const& pos, F32 lum) {
    ChkPos chk_pos = to_chk_pos(pos);
    ChkIdx chk_idx = to_chk_idx(pos);
    //@TODO
    light_nodes[chk_pos].push(
        LightNode{(F32)round(lum * (F32)LIGHT_RANGE), to_chk_idx(pos)});
    awaiting_light_updates.insert(chk_pos);
}

void del_light_node(MapPos const& pos) {
    //@TODO
    LUX_UNIMPLEMENTED();
}

static void update_chunk_light(ChkPos const &pos, Chunk& chunk) {
    if(light_nodes.count(pos) == 0) return;

    auto &nodes = light_nodes.at(pos);
    while(!nodes.empty()) {
        LightNode node = nodes.front();
        nodes.pop();

        MapPos base_pos = to_map_pos(pos, node.idx);
        //@IMPROVE bit-level parallelism

        LightLvl map_lvl = chunk.light_lvl[node.idx];
        U8 map_lum = (map_lvl & 0xF000) >> 12;
        if(chunk.blocks[node.idx].id != void_block) {
            node.lum = round((F32)node.lum * 0.5f);
        }
        if(map_lum < node.lum) {
            /* node.lum is guaranteed to be non-zero when > map_lum */
            U8 new_lum = node.lum;
            chunk.light_lvl[node.idx] &= 0x0FFF;
            chunk.light_lvl[node.idx] |= new_lum << 12;
            if(chunk.blocks[node.idx].id != void_block) {
                node.lum = 1;
            }
            if(node.lum >= 2) {
                U8 side_lum = node.lum - 1;
                for(auto const &offset : manhattan_hollow<MapCoord>) {
                    MapPos map_pos = base_pos + offset;
                    ChkPos chk_pos = to_chk_pos(map_pos);
                    ChkIdx idx     = to_chk_idx(map_pos);
                    if(chk_pos == pos) {
                        nodes.push({side_lum, idx});
                    } else {
                        light_nodes[chk_pos].push({side_lum, idx});
                        awaiting_light_updates.insert(chk_pos);
                    }
                }
            }
        }
    }
}

bool map_cast_ray(MapPos* out_pos, Vec3F* out_dir, Vec3F src, Vec3F dst) {
    Vec3F ray = dst - src;
    Vec3F a = abs(ray);
    int max_i;
    if(a.x > a.y) {
        if(a.x > a.z) max_i = 0;
        else          max_i = 2;
    } else {
        if(a.y > a.z) max_i = 1;
        else          max_i = 2;
    }
    F32 max = a[max_i];
    Vec3F dt = ray / max;
    Vec3F fr = src - trunc(src);
    F32    o = fr[max_i];

    Vec3F it = src + o * dt;

    for(Uns i = 0; i < max; ++i)
    {
        MapPos map_pos = floor(it);
        ChkPos chk_pos = to_chk_pos(map_pos);
        if(!is_chunk_loaded(chk_pos)) return false;
        if(get_chunk(chk_pos).blocks[to_chk_idx(map_pos)].lvl >= 0x8) {
            *out_pos = map_pos;
            Vec3F norm(0.f);
            norm[max_i] = sign(ray[max_i]);
            *out_dir = norm;
            return true;
        }

        it += dt;
    }
    return false;
}
