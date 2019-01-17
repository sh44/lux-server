#include <cstring>
#include <cstdlib>
//
#define GLM_FORCE_PURE
#include <glm/gtc/noise.hpp>
//
#include <lux_shared/common.hpp>
#include <lux_shared/map.hpp>
#include <lux_shared/noise.hpp>
//
#include <db.hpp>
#include <entity.hpp>
#include "map.hpp"

struct SuspendedChunk {
    BitArr<CHK_VOL> mask;
    Arr<BlockId, CHK_VOL> blocks;
};
static VecMap<ChkPos, Chunk> chunks;
static VecMap<ChkPos, SuspendedChunk> suspended_chunks;
static VecMap<Vec2<ChkCoord>, Arr<F32, CHK_SIZE * CHK_SIZE>> height_map;
F32 day_cycle;
VecSet<ChkPos> block_updated_chunks;
VecSet<ChkPos> light_updated_chunks;

constexpr Uns LIGHT_BITS_PER_COLOR = 4;
static_assert(LIGHT_BITS_PER_COLOR * 2 <= sizeof(LightLvl) * 8);
constexpr Uns LIGHT_RANGE          = 1 << LIGHT_BITS_PER_COLOR;

static void update_chunk_light(ChkPos const &pos, Chunk& chunk);
static bool is_chunk_loaded(ChkPos const& pos) {
    return chunks.count(pos) > 0;
}

static void write_suspended_block(MapPos const& pos, BlockId id) {
    ChkPos chk_pos = to_chk_pos(pos);
    ChkIdx chk_idx = to_chk_idx(pos);
    if(is_chunk_loaded(chk_pos)) {
        auto& chk = write_chunk(chk_pos);
        chk.blocks[chk_idx] = id;
    } else {
        auto& chk = suspended_chunks[chk_pos];
        chk.blocks[chk_idx] = id;
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
    constexpr Uns octaves    = 8;
    constexpr F32 base_scale = 0.002f;
    constexpr F32 h_exp      = 3.f;
    constexpr F32 max_h      = 256.f;
    Vec2<ChkCoord> h_pos = pos;
    if(height_map.count(h_pos) == 0) {
        auto& h_chunk = height_map[h_pos];
        Vec2F base_seed = (Vec2F)(h_pos * (ChkCoord)CHK_SIZE) * base_scale;
        Vec2F seed = base_seed;
        Uns idx = 0;
        for(Uns y = 0; y < CHK_SIZE; ++y) {
            for(Uns x = 0; x < CHK_SIZE; ++x) {
                h_chunk[idx] =
                    pow(p_norm(noise_fbm(seed, octaves)), h_exp) * max_h;
                seed.x += base_scale;
                ++idx;
            }
            seed.x  = base_seed.x;
            seed.y += base_scale;
        }
    }
    auto const& h_chunk = height_map.at(h_pos);
    for(Uns i = 0; i < CHK_VOL; ++i) {
        MapPos map_pos = to_map_pos(pos, i);
        F32 h = h_chunk[i & ((CHK_SIZE * CHK_SIZE) - 1)];
        MapCoord f_h = glm::floor(h);

        BlockId floor = map_pos.z > f_h ? void_block :
                        map_pos.z > 4 - f_h ? grass :
                        map_pos.z > 3 - f_h ? dark_grass :
                        map_pos.z > 0 - f_h ? dirt : raw_stone;
        F32 r_h = glm::clamp(h - (F32)map_pos.z, 0.f, 1.f);
        chunk.blocks[i] = floor | ((U16)(r_h * 15.f) << 8);
        /*if(glm::abs(glm::simplex((Vec3F)map_pos * 0.07f)) > 0.6f) {
            chunk.blocks[i] = void_block;
        }*/
        /*if(chunk.blocks[i] == void_block) {
            chunk.light_lvl[i] = 0x0F00;
        } else {
            chunk.light_lvl[i] = 0x0000;
        }
        if(chunk.blocks[i] == void_block && lux_randf(map_pos) > 0.999f) {
            add_light_node(to_map_pos(pos, i), 0.5f);
        }*/
    }
    for(Uns i = 0; i < CHK_VOL; ++i) {
        MapCoord h = glm::round(h_chunk[i & ((CHK_SIZE * CHK_SIZE) - 1)]);
        MapPos map_pos = to_map_pos(pos, i);
        if(map_pos.z == h && lux_randf(map_pos) > 0.99f) {
            for(Uns j = 0; j < 10; ++j) {
                write_suspended_block(map_pos + MapPos(0, 0, j), dirt | 0xFF00);
            }
            for(MapCoord z = -3; z <= 3; ++z) {
                for(MapCoord y = -5; y <= 5; ++y) {
                    for(MapCoord x = -5; x <= 5; ++x) {
                        if(glm::length(Vec3F(x, y, z)) < 5.f) {
                            write_suspended_block(map_pos + MapPos(x, y, 10 + z), dark_grass | 0xFF00);
                        }
                    }
                }
            }
        }
    }
    return chunk;
}

void guarantee_chunk(ChkPos const& pos) {
    if(!is_chunk_loaded(pos)) {
        load_chunk(pos);
    }
}

Chunk const& get_chunk(ChkPos const& pos) {
    LUX_ASSERT(is_chunk_loaded(pos));
    ///wish there was a non-bound-checking way to do it
    return chunks.at(pos);
}

Chunk& write_chunk(ChkPos const& pos) {
    LUX_ASSERT(is_chunk_loaded(pos));
    block_updated_chunks.emplace(pos);
    return chunks.at(pos);
}

BlockId get_block(MapPos const& pos) {
    return get_chunk(to_chk_pos(pos)).blocks[to_chk_idx(pos)];
}

BlockBp const& get_block_bp(MapPos const& pos) {
    return db_block_bp(get_block(pos));
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
            auto& dst = write_chunk(it->first);
            for(Uns i = 0; i < CHK_VOL; ++i) {
                if(src.mask[i]) dst.blocks[i] = src.blocks[i];
            }
            it = suspended_chunks.erase(it);
        } else ++it;
    }
}

void map_tick() {
    static Uns tick_num = 0;
    map_apply_suspended_updates();
    for(auto const& update : awaiting_light_updates) {
        if(is_chunk_loaded(update)) {
            light_updated_chunks.emplace(update);
        }
    }
    for(auto const& update : light_updated_chunks) {
        update_chunk_light(update, chunks.at(update));
        awaiting_light_updates.erase(update);
    }
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
        LightNode{(F32)glm::round(lum * (F32)LIGHT_RANGE), to_chk_idx(pos)});
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
        if(chunk.blocks[node.idx] != void_block) {
            node.lum = glm::round((F32)node.lum * 0.5f);
        }
        if(map_lum < node.lum) {
            /* node.lum is guaranteed to be non-zero when > map_lum */
            U8 new_lum = node.lum;
            chunk.light_lvl[node.idx] &= 0x0FFF;
            chunk.light_lvl[node.idx] |= new_lum << 12;
            if(chunk.blocks[node.idx] != void_block) {
                node.lum = 1;
            }
            if(node.lum >= 2) {
                U8 side_lum = node.lum - 1;
                for(auto const &offset : manhattan_hollow<MapCoord>) {
                    //@TODO don't spread lights through Z if there is floor
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
