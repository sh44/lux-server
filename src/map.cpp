#include <cstring>
#include <cstdlib>
//
#include <lux_shared/common.hpp>
#include <lux_shared/map.hpp>
#include <lux_shared/uninit_obj.hpp>
//
#include <physics.hpp>
#include <db.hpp>
#include <entity.hpp>
#include <chunk_loader.hpp>
#include <chunk_mesher.hpp>
#include "map.hpp"

struct ChunkPhysicsMesh {
    UninitObj<btTriangleIndexVertexArray> trigs_array;
    UninitObj<btBvhTriangleMeshShape>     shape;

    //for some reason we need to store these or bullet will crash the program
    DynArr<Vec3F> verts;
    DynArr<U32>   idxs;

    btRigidBody* body;

    ~ChunkPhysicsMesh() {
        physics_remove_body(body);
    }
};

static VecSet<ChkPos> mesher_requested_chunks;
static VecMap<ChkPos, Chunk> chunks;

F32 day_cycle;
VecSet<ChkPos> updated_chunks;
VecSet<ChkPos> updated_meshes;

//needs the out.pos set
static bool prepare_mesher_data(MesherRequest& out);
static bool is_chunk_loaded(ChkPos const& pos) {
    return chunks.count(pos) > 0;
}

static void write_suspended_block(MapPos const& pos, Block block) {
    ChkPos chk_pos = to_chk_pos(pos);
    ChkIdx chk_idx = to_chk_idx(pos);
    if(is_chunk_loaded(chk_pos)) {
        updated_chunks.insert(chk_pos);
        Chunk& chk = chunks.at(chk_pos);
        chk[chk_idx] = block;
        chk.updated_blocks.insert(chk_idx);
    } else {
        loader_write_suspended_block(block, pos);
    }
}

void map_init() {
    loader_init();
    mesher_init();
}

void map_deinit() {
    mesher_deinit();
    loader_deinit();
}

void guarantee_chunk(ChkPos const& pos) {
    if(!is_chunk_loaded(pos)) {
        ChkPos l_pos = pos;
        loader_enqueue_wait({&l_pos, 1});
        {   auto const& results = loader_lock_results();
            for(auto const& pair : results) {
                auto& chunk = chunks[pair.first];
                chunk.data = move(pair.second);
            }
            loader_unlock_results();
        }
    }
}

bool try_guarantee_chunk(ChkPos const& pos) {
    if(!is_chunk_loaded(pos)) {
        ChkPos l_pos = pos;
        loader_enqueue({&l_pos, 1});
        return false;
    }
    return true;
}

ChunkMesh::ChunkMesh(ChunkMesh&& that) {
    faces = move(that.faces);
    physics_mesh = that.physics_mesh;
    that.physics_mesh = nullptr;
}

Block &Chunk::operator[](ChkIdx idx) {
    return data->blocks[idx];
}

Block const &Chunk::operator[](ChkIdx idx) const {
    return data->blocks[idx];
}

Chunk::~Chunk() {
    switch(mesh_state) {
        case BUILT_PHYSICS: {
            delete mesh->physics_mesh;
        }
        [[fallthrough]];
        case BUILT_TRIANGLE: {
            delete mesh;
        }
        case BUILT_EMPTY:
        case NOT_BUILT: break;
    }
}

void enqueue_missing_chunks_meshes(VecSet<ChkPos> const& requests) {
    /*static DynArr<ChkPos> loader_requests;
    loader_requests.clear();
    loader_requests.reserve_at_least(requests.size());

    for(auto const& request : requests) {
        if(mesher_requested_chunks.count(pos) == 0) {
            loader_requests.push(pos);
        }
    }*/
}

bool try_guarantee_chunk_mesh(ChkPos const& pos) {
    if(mesher_requested_chunks.count(pos) > 0) {
        return false;
    }
    Arr<ChkPos, 4> positions;
    Slice<ChkPos> slice = {positions, 0};
    if(is_chunk_loaded(pos)) {
        Chunk& chunk = chunks.at(pos);
        if(chunk.mesh_state != Chunk::NOT_BUILT) {
            return true;
        }
    } else {
        slice[slice.len++] = pos;
    }
    Arr<ChkPos, 3> offs {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    for(ChkPos const& off : offs) {
        if(!is_chunk_loaded(pos + off)) {
            slice[slice.len++] = pos + off;
        }
    }
    if(slice.len > 0) {
        loader_enqueue(slice);
        return false;
    }
    DynArr<MesherRequest> mesher_requests(1);
    auto& request = mesher_requests[0];
    request.pos = pos;
    if(not prepare_mesher_data(request)) {
        ///empty mesh detected
        auto& chunk = chunks.at(pos);
        chunk.mesh_state = Chunk::BUILT_EMPTY;
        return true;
    } else {
        mesher_enqueue(move(mesher_requests));
        mesher_requested_chunks.insert(pos);
        return false;
    }
}

static void chunk_physics_mesh_build(ChkPos const& pos) {
    auto& chunk = chunks.at(pos);
    if(chunk.mesh_state == Chunk::BUILT_TRIANGLE) {
        auto& mesh = *chunk.mesh;
        chunk.mesh->physics_mesh = new ChunkPhysicsMesh();
        auto& p_mesh = *chunk.mesh->physics_mesh;
        Uns faces_num = mesh.faces.len;
        LUX_ASSERT(faces_num > 0);
        p_mesh.verts.resize(faces_num * 4);
        p_mesh.idxs.resize(faces_num * 6);
        Arr<Vec3F, 3 * 4> vert_offs = {
            {0, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 1, 1},
            {0, 0, 0}, {0, 0, 1}, {1, 0, 0}, {1, 0, 1},
            {0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 0},
        };
        for(Uns i = 0; i < faces_num; ++i) {
            auto const& face = mesh.faces[i];
            ChkIdx idx = face.idx;
            U8 axis = (face.orientation & 0b110) >> 1;
            LUX_ASSERT(axis != 0b11);
            U8 sign = (face.orientation & 1);
            Vec3F face_off(0);
            face_off[axis] = 1;
            if(sign) {
                for(Uns j = 0; j < 6; ++j) {
                    p_mesh.idxs[i * 6 + j] = quad_idxs<U16>[j] + i * 4;
                }
            } else {
                for(Uns j = 0; j < 6; ++j) {
                    p_mesh.idxs[i * 6 + j] = quad_idxs<U16>[5 - j] + i * 4;
                }
            }
            for(Uns j = 0; j < 4; ++j) {
                auto& vert = p_mesh.verts[i * 4 + j];
                vert = (Vec3F)to_idx_pos(idx) + face_off +
                    vert_offs[axis * 4 + j];
            }
        }
        p_mesh.trigs_array.init(
            faces_num * 2, (I32*)p_mesh.idxs.beg , sizeof(I32) * 3,
            p_mesh.verts.len, (F32*)p_mesh.verts.beg, sizeof(Vec3F));

        p_mesh.shape.init(
            &*p_mesh.trigs_array, true, btVector3{0, 0, 0},
            btVector3{CHK_SIZE, CHK_SIZE, CHK_SIZE});

        p_mesh.body = physics_create_mesh(to_map_pos(pos, 0), &*p_mesh.shape);
        chunk.mesh_state = Chunk::BUILT_PHYSICS;
    }
}

void guarantee_physics_mesh_for_aabb(MapPos const& min, MapPos const& max) {
    ChkPos const c_min = to_chk_pos(min);
    ChkPos const c_max = to_chk_pos(max);
    LUX_ASSERT(glm::all(glm::lessThanEqual(c_min, c_max)));
    ChkPos iter;

    //first we do a quick discard of already built meshes (most of the cases)
    bool needs_generation = false;
    for(iter.z = c_min.z; iter.z <= c_max.z; ++iter.z) {
        for(iter.y = c_min.y; iter.y <= c_max.y; ++iter.y) {
            for(iter.x = c_min.x; iter.x <= c_max.x; ++iter.x) {
                if(!is_chunk_loaded(iter) ||
                  (get_chunk(iter).mesh_state != Chunk::BUILT_PHYSICS &&
                   get_chunk(iter).mesh_state != Chunk::BUILT_EMPTY)) {
                    needs_generation = true;
                }
            }
        }
    }
    if(not needs_generation) return;

    static DynArr<ChkPos> loader_requests;
    loader_requests.clear();
    ChkPos const c_size = (c_max + (ChkCoord)1) - c_min;
    loader_requests.reserve_at_least(glm::compMul(c_size) +
        c_size.x * c_size.y + c_size.x * c_size.z + c_size.y * c_size.z);
    for(iter.z = c_min.z; iter.z <= c_max.z; ++iter.z) {
        for(iter.y = c_min.y; iter.y <= c_max.y; ++iter.y) {
            for(iter.x = c_min.x; iter.x <= c_max.x; ++iter.x) {
                if(!is_chunk_loaded(iter)) {
                    loader_requests.emplace(iter);
                }
            }
        }
    }
    iter.x = c_max.x + (ChkCoord)1;
    for(iter.z = c_min.z; iter.z <= c_max.z; ++iter.z) {
        for(iter.y = c_min.y; iter.y <= c_max.y; ++iter.y) {
            if(!is_chunk_loaded(iter)) {
                loader_requests.emplace(iter);
            }
        }
    }
    iter.y = c_max.y + (ChkCoord)1;
    for(iter.z = c_min.z; iter.z <= c_max.z; ++iter.z) {
        for(iter.x = c_min.x; iter.x <= c_max.x; ++iter.x) {
            if(!is_chunk_loaded(iter)) {
                loader_requests.emplace(iter);
            }
        }
    }
    iter.z = c_max.z + (ChkCoord)1;
    for(iter.y = c_min.y; iter.y <= c_max.y; ++iter.y) {
        for(iter.x = c_min.x; iter.x <= c_max.x; ++iter.x) {
            if(!is_chunk_loaded(iter)) {
                loader_requests.emplace(iter);
            }
        }
    }
    if(loader_requests.len > 0) {
        loader_enqueue_wait(loader_requests);
        auto const& results = loader_lock_results();
        for(auto const& pair : results) {
            auto& chunk = chunks[pair.first];
            chunk.data = pair.second;
        }
        loader_unlock_results();
    }

    //now we build the meshes
    static DynArr<MesherRequest> mesher_requests;
    mesher_requests.reserve_at_least(glm::compMul(c_size));
    for(iter.z = c_min.z; iter.z <= c_max.z; ++iter.z) {
        for(iter.y = c_min.y; iter.y <= c_max.y; ++iter.y) {
            for(iter.x = c_min.x; iter.x <= c_max.x; ++iter.x) {
                auto& chunk = chunks.at(iter);
                if(chunk.mesh_state == Chunk::NOT_BUILT) {
                    auto& request = mesher_requests.emplace();
                    request.pos = iter;
                    if(not prepare_mesher_data(request)) {
                        //empty mesh
                        chunk.mesh_state = Chunk::BUILT_EMPTY;
                        mesher_requests.erase(mesher_requests.len - 1);
                    }
                }
            }
        }
    }
    if(mesher_requests.len > 0) {
        mesher_enqueue_wait(move(mesher_requests));
        auto& meshes = mesher_lock_results();
        for(auto&& pair : meshes) {
            auto& chunk = chunks.at(pair.first);
            LUX_ASSERT(chunk.mesh_state == Chunk::NOT_BUILT);
            chunk.mesh = new ChunkMesh(move(pair.second));
            LUX_ASSERT(chunk.mesh->faces.len > 0);
            chunk.mesh_state = Chunk::BUILT_TRIANGLE;
            mesher_requested_chunks.erase(pair.first);
        }
        mesher_unlock_results();
    }

    //finally we build the physics meshes
    for(iter.z = c_min.z; iter.z <= c_max.z; ++iter.z) {
        for(iter.y = c_min.y; iter.y <= c_max.y; ++iter.y) {
            for(iter.x = c_min.x; iter.x <= c_max.x; ++iter.x) {
                chunk_physics_mesh_build(iter);
            }
        }
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
    return chunk[chk_idx];
}

Block get_block(MapPos const& pos) {
    return get_chunk(to_chk_pos(pos))[to_chk_idx(pos)];
}

BlockBp const& get_block_bp(MapPos const& pos) {
    return db_block_bp(get_block(pos).id);
}

static void chunk_mesh_update(ChkPos const& chk_pos);

void map_tick() {
    benchmark("tick", 1.0 / 64.0, [&](){
    {   LoaderResults* results;
        if(loader_try_lock_results(results)) {
            for(auto const& pair : *results) {
                auto& chunk = chunks[pair.first];
                chunk.data = pair.second;
            }
            loader_unlock_results();
        }
    }
    {   LoaderBlockChanges* block_changes;
        if(loader_try_lock_block_changes(block_changes)) {
            for(auto it = block_changes->begin(), end = block_changes->end();
                     it != end;) {
                if(is_chunk_loaded(it->first)) {
                    auto& chunk = chunks.at(it->first);
                    updated_chunks.insert(it->first);
                    for(auto const& change : it->second) {
                        chunk[change.idx] = change.block;
                        chunk.updated_blocks.insert(change.idx);
                    }
                    it = block_changes->erase(it);
                } else ++it;
            }
            loader_unlock_block_changes();
        }
    }
    {   MesherResults* results;
        if(mesher_try_lock_results(results)) {
            MesherResults& bob = *results;
            for(auto&& pair : bob) {
                auto& chunk = chunks.at(pair.first);
                LUX_ASSERT(chunk.mesh_state == Chunk::NOT_BUILT);
                chunk.mesh = new ChunkMesh(move(pair.second));
                chunk.mesh_state = Chunk::BUILT_TRIANGLE;
                mesher_requested_chunks.erase(pair.first);
            }
            mesher_unlock_results();
        }
    }
    });

    benchmark("chunk updates", 1.0 / 64.0, [&](){
    for(auto const& pos : updated_chunks) {
        //@URGENT
        //LUX_UNIMPLEMENTED(); //@TODO physics
        /*if(physics_meshes.count(pos) > 0) {
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
        }*/
        chunk_mesh_update(pos);
    }
    });
    updated_chunks.clear();
    static Uns tick_num = 0;
    physics_tick(1.f / 64.f); //TODO tick time
    Uns constexpr ticks_per_day = 64 * 60 * 24;
    day_cycle = std::sin(tau *
        (((F32)(tick_num % ticks_per_day) / (F32)ticks_per_day) + 0.25f));
    tick_num++;
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

    for(Uns i = 0; i < max; ++i) {
        for(Uns j = 0; j < 3; ++j) {
            MapPos map_pos = floor(it);
            ChkPos chk_pos = to_chk_pos(map_pos);
            if(!is_chunk_loaded(chk_pos)) return false;
            if(get_chunk(chk_pos)[to_chk_idx(map_pos)].id != void_block) {
                *out_pos = map_pos;
                Vec3F norm(0.f);
                norm[max_i] = sign(ray[max_i]);
                *out_dir = norm;
                return true;
            }
            it[j] += dt[j];
        }
    }
    return false;
}

static void chunk_mesh_update(ChkPos const& chk_pos) {
    LUX_ASSERT(is_chunk_loaded(chk_pos));
    Chunk& chunk = chunks.at(chk_pos);
    if(chunk.mesh_state == Chunk::NOT_BUILT) return;
    if(chunk.mesh_state == Chunk::BUILT_EMPTY) {
        LUX_UNIMPLEMENTED();
        return;
    }
    ChunkMesh& mesh = *chunk.mesh;
    for(auto const& idx : chunk.updated_blocks) {
        IdxPos i_pos = to_idx_pos(idx);
        auto const& b0 = chunk[idx];
        Uns removed_count = 0;
        //@TODO in this function, we will probably need something more efficient
        //than linear search of the faces
        for(Uns i = 0; i < mesh.faces.len;) {
            auto const& face = mesh.faces[i];
            if(face.idx == idx) {
                mesh.faces.erase(i);
                mesh.removed_faces.emplace(i);
                updated_meshes.insert(chk_pos);
                removed_count++;
            } else ++i;
            if(removed_count == 3) break;
        }
        for(Uns a = 0; a < 3; ++a) {
            if(i_pos[a] == 0) {
                ChkPos off_pos = chk_pos;
                off_pos[a]--;
                if(is_chunk_loaded(off_pos)) {
                    Chunk& off_chunk = chunks.at(off_pos);
                    if(off_chunk.mesh_state != Chunk::NOT_BUILT) {
                        if(off_chunk.mesh_state == Chunk::BUILT_EMPTY) {
                            //@URGENT
                    //        LUX_UNIMPLEMENTED();
                            continue;
                        }
                        ChunkMesh& off_mesh = *off_chunk.mesh;
                        IdxPos off_i_pos = i_pos;
                        off_i_pos[a] = CHK_SIZE - 1;
                        for(Uns i = 0; i < off_mesh.faces.len;) {
                            auto const& face = off_mesh.faces[i];
                            if(to_idx_pos(face.idx) == off_i_pos &&
                               (face.orientation & 0b110) >> 1 == a) {
                                off_mesh.faces.erase(i);
                                off_mesh.removed_faces.emplace(i);
                                updated_meshes.insert(off_pos);
                                break;
                            } else ++i;
                        }
                        ChkIdx off_idx = to_chk_idx(off_i_pos);
                        auto const& b1 = off_chunk[off_idx];
                        if((b0.id == void_block) != (b1.id == void_block)) {
                            U8 orient = b0.id == void_block;
                            BlockId id = not orient ? b0.id : b1.id;
                            off_mesh.faces.push({off_idx, id, (((U8)a << 1) | orient)});
                            off_mesh.added_faces.emplace(off_mesh.faces.last());
                            updated_meshes.insert(off_pos);
                        }
                    }
                }
                {   IdxPos off_i_pos = i_pos;
                    off_i_pos[a]++;
                    auto const& b1 = chunk[to_chk_idx(off_i_pos)];
                    if((b0.id == void_block) != (b1.id == void_block)) {
                        U8 orient = b0.id != void_block;
                        BlockId id = orient ? b0.id : b1.id;
                        mesh.faces.push({idx, id, (((U8)a << 1) | orient)});
                        mesh.added_faces.emplace(mesh.faces.last());
                        updated_meshes.insert(chk_pos);
                    }
                }
            } else {
                {   IdxPos off_i_pos = i_pos;
                    off_i_pos[a]--;
                    ChkIdx off_idx = to_chk_idx(off_i_pos);
                    for(Uns i = 0; i < mesh.faces.len;) {
                        auto const& face = mesh.faces[i];
                        if(face.idx == off_idx &&
                           (face.orientation & 0b110) >> 1 == a) {
                            mesh.faces.erase(i);
                            mesh.removed_faces.emplace(i);
                            updated_meshes.insert(chk_pos);
                            break;
                        } else ++i;
                    }
                    auto const& b1 = chunk[off_idx];
                    if((b0.id == void_block) != (b1.id == void_block)) {
                        U8 orient = b0.id == void_block;
                        BlockId id = not orient ? b0.id : b1.id;
                        mesh.faces.push({off_idx, id, (((U8)a << 1) | orient)});
                        mesh.added_faces.emplace(mesh.faces.last());
                        updated_meshes.insert(chk_pos);
                    }
                }
                if(i_pos[a] == CHK_SIZE - 1) {
                    ChkPos off_pos = chk_pos;
                    off_pos[a]++;
                    LUX_ASSERT(is_chunk_loaded(off_pos));
                    Chunk& off_chunk = chunks.at(off_pos);
                    IdxPos off_i_pos = i_pos;
                    off_i_pos[a] = 0;
                    auto const& b1 = off_chunk[to_chk_idx(off_i_pos)];
                    if((b0.id == void_block) != (b1.id == void_block)) {
                        U8 orient = b0.id != void_block;
                        BlockId id = orient ? b0.id : b1.id;
                        mesh.faces.push({idx, id, (((U8)a << 1) | orient)});
                        mesh.added_faces.emplace(mesh.faces.last());
                        updated_meshes.insert(chk_pos);
                    }
                } else {
                    IdxPos off_i_pos = i_pos;
                    off_i_pos[a]++;
                    auto const& b1 = chunk[to_chk_idx(off_i_pos)];
                    if((b0.id == void_block) != (b1.id == void_block)) {
                        U8 orient = b0.id != void_block;
                        BlockId id = orient ? b0.id : b1.id;
                        mesh.faces.push({idx, id, (((U8)a << 1) | orient)});
                        mesh.added_faces.emplace(mesh.faces.last());
                        updated_meshes.insert(chk_pos);
                    }
                }
            }
        }
    }
    chunk.updated_blocks.clear();
}

static bool prepare_mesher_data(MesherRequest& out) {
    ChkPos const& pos = out.pos;
    bool has_any_faces = false;
    bool face_check;
    auto get_block_l = [&](Vec3I pos) -> Block& {
        Uns idx = pos.x + (pos.y + pos.z * (CHK_SIZE + 1)) * (CHK_SIZE + 1);
        LUX_ASSERT(idx < arr_len(out.blocks));
        return out.blocks[idx];
    };
    {   Chunk const& chk = get_chunk(pos + ChkPos(1, 0, 0));
        {   auto const& block =
                chk[to_chk_idx(IdxPos{0, 0, 0})];
            get_block_l({CHK_SIZE, 0, 0}) = block;
            face_check = block.id == void_block;
        }
        for(Uns y = 1; y < CHK_SIZE; ++y) {
            auto const& block = chk[to_chk_idx(IdxPos{0, y, 0})];
            get_block_l({CHK_SIZE, y, 0}) = block;
            has_any_faces |= (block.id == void_block) != face_check;
        }
        for(Uns z = 1; z < CHK_SIZE; ++z) {
            for(Uns y = 0; y < CHK_SIZE; ++y) {
                auto const& block = chk[to_chk_idx(IdxPos{0, y, z})];
                get_block_l({CHK_SIZE, y, z}) = block;
                has_any_faces |= (block.id == void_block) != face_check;
            }
        }
    }
    {   Chunk const& chk = get_chunk(pos + ChkPos(0, 1, 0));
        for(Uns z = 0; z < CHK_SIZE; ++z) {
            for(Uns x = 0; x < CHK_SIZE; ++x) {
                auto const& block = chk[to_chk_idx(IdxPos{x, 0, z})];
                get_block_l({x, CHK_SIZE, z}) = block;
                has_any_faces |= (block.id == void_block) != face_check;
            }
        }
    }
    {   Chunk const& chk = get_chunk(pos + ChkPos(0, 0, 1));
        for(Uns y = 0; y < CHK_SIZE; ++y) {
            for(Uns x = 0; x < CHK_SIZE; ++x) {
                auto const& block = chk[to_chk_idx(IdxPos{x, y, 0})];
                get_block_l({x, y, CHK_SIZE}) = block;
                has_any_faces |= (block.id == void_block) != face_check;
            }
        }
    }
    Chunk const& chk = get_chunk(pos);
    Uns src = 0;
    Uns dst = 0;
    for(Uns z = 0; z < CHK_SIZE; ++z) {
        for(Uns y = 0; y < CHK_SIZE; ++y) {
            for(Uns x = 0; x < CHK_SIZE; ++x) {
                auto const& block = chk[src];
                out.blocks[dst] = block;
                has_any_faces |= (block.id == void_block) != face_check;
                src++;
                dst++;
            }
            dst++;
        }
        dst += CHK_SIZE + 1;
    }
    return has_any_faces;
}
