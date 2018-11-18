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
VecSet<ChkPos> tile_updated_chunks;
VecSet<ChkPos> light_updated_chunks;

constexpr Uns LIGHT_BITS_PER_COLOR = 5;
static_assert(LIGHT_BITS_PER_COLOR * 3 <= sizeof(LightLvl) * 8);
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
    static const TileId raw_stone  = db_tile_id("raw_stone");
    static const TileId grass      = db_tile_id("grass");
    static const TileId dark_grass = db_tile_id("dark_grass");
    static const TileId dirt       = db_tile_id("dirt");
    constexpr Uns octaves    = 16;
    constexpr F32 base_scale = 0.015f;
    for(Uns i = 0; i < CHK_VOL; ++i) {
        MapPos map_pos = to_map_pos(pos, i);
        chunk.light_lvl[i] = 0xFFFF;
        F32 scale  = base_scale;
        F32 weight = 1.f;
        F32 avg    = 0.f;
        for(Uns o = 0; o < octaves; ++o) {
            avg    += glm::simplex((Vec2F)map_pos * scale) * weight;
            scale  *= 2.f;
            weight /= 2.f;
        }
        F32 h = ((avg / (2.f - (weight * 2.f))) + 1.f) / 2.f;

        chunk.wall[i] = h > 0.5f ? raw_stone : void_tile;
        chunk.roof[i] = chunk.wall[i];
        if(chunk.wall[i] != void_tile &&
           glm::simplex((Vec2F)map_pos * 0.025f) > 0.7f) {
            chunk.wall[i] = void_tile;
            chunk.floor[i] = dirt;
        } else {
            chunk.floor[i] = h > 0.5f   ? raw_stone :
                             h > 0.45f  ? dirt :
                             h > 0.3f   ? grass :
                             h > 0.125f ? dark_grass : dirt;
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
    Vec3<U8> col;
    ChkIdx   idx;
    static_assert(sizeof(col.r) * 8 >= LIGHT_BITS_PER_COLOR);
};

VecMap<ChkPos, Queue<LightNode>> light_nodes;
VecSet<ChkPos>                   awaiting_light_updates;

void map_tick() {
    for(auto const& update : awaiting_light_updates) {
        if(is_chunk_loaded(update)) {
            light_updated_chunks.emplace(update);
        }
    }
    for(auto const& update : light_updated_chunks) {
        update_chunk_light(update, chunks.at(update));
        awaiting_light_updates.erase(update);
    }
    light_updated_chunks.clear();
    tile_updated_chunks.clear();
}

void add_light_node(MapPos const& pos, Vec3F const& col) {
    ChkPos chk_pos = to_chk_pos(pos);
    ChkIdx chk_idx = to_chk_idx(pos);
    //@TODO
    light_nodes[chk_pos].push(LightNode{
        Vec3<U8>(glm::round(col * (F32)LIGHT_RANGE)), to_chk_idx(pos)});
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
        Vec3<U8> map_color = {(map_lvl & 0xF800) >> 11,
                              (map_lvl & 0x07C0) >>  6,
                              (map_lvl & 0x003E) >>  1};
        if(chunk.wall[node.idx] != void_tile) {
            node.col = (Vec3<U8>)glm::round(Vec3F(node.col) * 0.5f);
        }
        auto is_less = glm::lessThan(map_color, node.col);
        if(glm::any(is_less)) {
            /* node.col is guaranteed to be non-zero when is_less is true */
            Vec3<U8> new_color = node.col * (Vec3<U8>)is_less +
                                 map_color * (Vec3<U8>)(glm::not_(is_less));
            chunk.light_lvl[node.idx] = (new_color.r << 11) |
                                        (new_color.g <<  6) |
                                        (new_color.b <<  1);
            if(chunk.wall[node.idx] != void_tile) {
                node.col = Vec3<U8>(1);
            }
            auto atleast_two = glm::greaterThanEqual(node.col, Vec3<U8>(2u));
            if(glm::any(atleast_two)) {
                Vec3<U8> side_color = node.col - (Vec3<U8>)atleast_two;
                for(auto const &offset : manhattan_hollow<MapCoord>) {
                    //@TODO don't spread lights through Z if there is floor
                    MapPos map_pos = base_pos + offset;
                    ChkPos chk_pos = to_chk_pos(map_pos);
                    ChkIdx idx     = to_chk_idx(map_pos);
                    if(chk_pos == pos) {
                        nodes.push({side_color, idx});
                    } else {
                        light_nodes[chk_pos].push({side_color, idx});
                        awaiting_light_updates.insert(chk_pos);
                    }
                }
            }
        }
    }
}
