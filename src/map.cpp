#include <cstring>
#include <cstdlib>
//
#define GLM_FORCE_PURE
#include <glm/gtc/noise.hpp>
//
#include <lux_shared/common.hpp>
#include <lux_shared/map.hpp>
//
#include <db.hpp>
#include <entity.hpp>
#include "map.hpp"

static VecMap<ChkPos, Chunk> chunks;
F32 day_cycle;
VecSet<ChkPos> tile_updated_chunks;
VecSet<ChkPos> light_updated_chunks;

constexpr Uns LIGHT_BITS_PER_COLOR = 4;
static_assert(LIGHT_BITS_PER_COLOR * 2 <= sizeof(LightLvl) * 8);
constexpr Uns LIGHT_RANGE          = 1 << LIGHT_BITS_PER_COLOR;

static void update_chunk_light(ChkPos const &pos, Chunk& chunk);
static bool is_chunk_loaded(ChkPos const& pos) {
    return chunks.count(pos) > 0;
}

static Chunk& load_chunk(ChkPos const& pos) {
    LUX_ASSERT(!is_chunk_loaded(pos));
    LUX_LOG("loading chunk");
    LUX_LOG("    pos: {%zd, %zd}", pos.x, pos.y);
    ///@RESEARCH to do a better way to no-copy default construct
    Chunk& chunk = chunks[pos];
    static const TileId raw_stone  = db_tile_id("raw_stone"_l);
    static const TileId wall       = db_tile_id("stone_wall"_l);
    static const TileId grass      = db_tile_id("grass"_l);
    static const TileId dark_grass = db_tile_id("dark_grass"_l);
    static const TileId dirt       = db_tile_id("dirt"_l);
    static const TileId leaves     = db_tile_id("tree_leaves"_l);
    constexpr Uns octaves    = 16;
    constexpr F32 base_scale = 0.015f;
    constexpr F32 mul        = 2.f;
    constexpr F32 h_exp      = 1.25f;
    for(Uns i = 0; i < CHK_VOL; ++i) {
        MapPos map_pos = to_map_pos(pos, i) + MapPos(1000, 1030);
        F32 scale  = base_scale;
        F32 weight = 1.f;
        F32 sum    = 0.f;
        F32 num    = 0.f;
        for(Uns o = 0; o < octaves; ++o) {
            auto rs = [&](){return glm::pow(lux_randf(map_pos, o), 2.f);};
            Vec2F shift = {rs(), rs()};
            shift  *= scale * 4.f;
            sum    += glm::simplex(((Vec2F)map_pos + shift) * scale) * weight;
            num    += weight;
            scale  *= mul;
            weight /= mul;
        }
        F32 h = glm::pow(((sum / num) + 1.f) / 2.f, h_exp);

        chunk.wall[i] = h > 0.5f ? wall : void_tile;
        if(chunk.roof[i] != leaves) {
            chunk.roof[i] = h > 0.47f ? raw_stone : void_tile;
        }
        if(chunk.wall[i] != void_tile &&
           glm::simplex((Vec2F)map_pos * 0.025f) > 0.7f) {
            chunk.wall[i] = void_tile;
            chunk.floor[i] = dirt;
        } else {
            chunk.floor[i] = h > 0.5f   ? dirt :
                             h > 0.45f  ? dirt :
                             h > 0.3f   ? grass :
                             h > 0.125f ? dark_grass : dirt;
        }
        if(chunk.wall[i] == void_tile) {
            chunk.light_lvl[i] = 0x0F00;
        } else {
            chunk.light_lvl[i] = 0x0000;
        }
        //placeholder trees
        /*IdxPos idx_pos = to_idx_pos(i);
        if(idx_pos.x > 0 && idx_pos.x < CHK_SIZE - 1 &&
           idx_pos.y > 0 && idx_pos.y < CHK_SIZE - 1 &&
           chunk.roof[i] == void_tile && chunk.wall[i] == void_tile &&
           lux_randf(map_pos) > 0.96f) {
            chunk.wall[i] = db_tile_id("tree_trunk");
            for(Uns y = idx_pos.y - 1; y <= idx_pos.y + 1; y++) {
                for(Uns x = idx_pos.x - 1; x <= idx_pos.x + 1; x++) {
                    ChkIdx idx = to_chk_idx(IdxPos(x, y));
                    if(chunk.roof[idx] == void_tile) {
                        chunk.roof[idx] = leaves;
                    }
                }
            }
        }*/
        if(chunk.wall[i] == void_tile && lux_randf(map_pos) > 0.999f) {
            add_light_node(to_map_pos(pos, i), 0.5f);
        }
    }
    if(pos == ChkPos(1, -1)) {
        ChkIdx idx;
        Uns tries = 0;
        do {
            idx = lux_randmm(0, CHK_VOL, pos, tries);
            tries++;
        } while(chunk.wall[idx] != void_tile && tries < 1000);
        MapPos map_pos = to_map_pos(pos, idx);
        EntityId id = create_entity();
        entity_comps.pos[id] = map_pos;
        entity_comps.vel[id] = {0.f, 0.f};
        entity_comps.shape[id] = EntityComps::Shape
            {{.sphere = {1.f}}, .tag = EntityComps::Shape::SPHERE};
        entity_comps.visible[id] = {0};
        entity_comps.orientation[id] = {{0.f, 0.f}, 0.f};
        entity_comps.a_vel[id] = {0.f, 0.1f};
        /*entity_comps.rasen[id].m[RN_RAM_LEN - 1] = id;
        U16 constexpr run_program[] = {
            RN_PUSHV(1),
            RN_LOADV(RN_R0, 0xff),
            RN_PUSH (RN_R0),
            RN_XCALL(RN_XC_ENTITY_MOVE),
            RN_LOADV(RN_R1, 0x00),
            RN_ADDV (RN_R1, 1),
            RN_CONST(RN_R2, 8),
            RN_MOD  (RN_R2, RN_R1),
            RN_SAVEV(RN_R1, 0x00),
            RN_COPY (RN_R1, RN_CR),
            RN_JNZ  (14),
            RN_PUSHV(1),
            RN_PUSH (RN_R0),
            RN_XCALL(RN_XC_ENTITY_ROTATE),
            RN_XCALL(RN_XC_HALT),
        };
        entity_comps.rasen[id].running = true;
        std::memcpy(entity_comps.rasen[id].o, run_program, sizeof(run_program));*/
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
    tile_updated_chunks.emplace(pos);
    return chunks.at(pos);
}

TileId get_floor(MapPos const& pos) {
    return get_chunk(to_chk_pos(pos)).floor[to_chk_idx(pos)];
}

TileId get_wall(MapPos const& pos) {
    return get_chunk(to_chk_pos(pos)).wall[to_chk_idx(pos)];
}

TileId get_roof(MapPos const& pos) {
    return get_chunk(to_chk_pos(pos)).roof[to_chk_idx(pos)];
}

TileBp const& get_floor_bp(MapPos const& pos) {
    return db_tile_bp(get_floor(pos));
}

TileBp const& get_wall_bp(MapPos const& pos) {
    return db_tile_bp(get_wall(pos));
}

TileBp const& get_roof_bp(MapPos const& pos) {
    return db_tile_bp(get_roof(pos));
}

//@CONSIDER separate file for lightsim
struct LightNode {
    U8       lum;
    ChkIdx   idx;
    static_assert(sizeof(lum) * 8 >= LIGHT_BITS_PER_COLOR);
};

VecMap<ChkPos, Queue<LightNode>> light_nodes;
VecSet<ChkPos>                   awaiting_light_updates;

void map_tick() {
    static Uns tick_num = 0;
    light_updated_chunks.clear();
    tile_updated_chunks.clear();
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
        if(chunk.wall[node.idx] != void_tile) {
            node.lum = glm::round((F32)node.lum * 0.5f);
        }
        if(map_lum < node.lum) {
            /* node.lum is guaranteed to be non-zero when > map_lum */
            U8 new_lum = node.lum;
            chunk.light_lvl[node.idx] &= 0x0FFF;
            chunk.light_lvl[node.idx] |= new_lum << 12;
            if(chunk.wall[node.idx] != void_tile) {
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
