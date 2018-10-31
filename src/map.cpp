#include <cstring>
#include <cstdlib>
//
#include <lux_shared/common.hpp>
#include <lux_shared/map.hpp>
//
#include <db.hpp>
#include <entity.hpp>
#include "map.hpp"

static VecMap<ChkPos, Chunk> chunks;

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
    TileId wall_id = db_tile_id("stone_wall");
    TileId floor_id = db_tile_id("stone_floor");
    for(Uns i = 0; i < CHK_VOL; ++i) {
        MapPos map_pos = to_map_pos(pos, i);
        if(lux_rands(3, map_pos / (MapCoord)16) == 0) {
            chunk.wall[i] = true;
        } else {
            chunk.wall[i] = false;
        }
        chunk.light_lvl[i] = 0x0000;
        chunk.fg_id[i] = wall_id;
        chunk.bg_id[i] = floor_id;
        if((map_pos.x % 16 != 0 && map_pos.y % 16 != 0) && rand() % 200 == 0) {
            add_light_node(to_map_pos(pos, i), {0.75f, 0.75f, 0.75f});
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

TileId get_fg_id(MapPos const& pos) {
    return get_chunk(to_chk_pos(pos)).fg_id[to_chk_idx(pos)];
}

TileId get_bg_id(MapPos const& pos) {
    return get_chunk(to_chk_pos(pos)).bg_id[to_chk_idx(pos)];
}

TileBp const& get_bg_bp(MapPos const& pos) {
    return db_tile_bp(get_bg_id(pos));
}

TileBp const& get_fg_bp(MapPos const& pos) {
    return db_tile_bp(get_fg_id(pos));
}

bool is_tile_wall(MapPos const& pos) {
    return get_chunk(to_chk_pos(pos)).wall[to_chk_idx(pos)];
}
//@CONSIDER separate file for lightsim
struct LightNode {
    Vec3<U8> col;
    ChkIdx   idx;
    static_assert(sizeof(col.r) * 8 >= LIGHT_BITS_PER_COLOR);
};

VecMap<ChkPos, Queue<LightNode>> light_nodes;
VecSet<ChkPos>                   awaiting_light_updates;

void map_tick(DynArr<ChkPos>& light_updated_chunks) {
    LUX_ASSERT(light_updated_chunks.size() == 0);
    for(auto const& update : awaiting_light_updates) {
        if(is_chunk_loaded(update)) {
            light_updated_chunks.emplace_back(update);
        }
    }
    for(auto const& update : light_updated_chunks) {
        update_chunk_light(update, chunks.at(update));
        awaiting_light_updates.erase(update);
    }
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

        constexpr MapPos offsets[4] =
            {{-1,  0}, { 1,  0},
             { 0, -1}, { 0,  1}};

        MapPos base_pos = to_map_pos(pos, node.idx);
        //@IMPROVE bit-level parallelism

        LightLvl map_lvl = chunk.light_lvl[node.idx];
        Vec3<U8> map_color = {(map_lvl & 0xF800) >> 11,
                              (map_lvl & 0x07C0) >>  6,
                              (map_lvl & 0x003E) >>  1};
        if(chunk.wall[node.idx]) {
            node.col = (Vec3<U8>)glm::round(Vec3F(node.col) * 0.5f);
        }
        auto is_less = glm::lessThan(map_color, node.col);
        auto atleast_two = glm::greaterThanEqual(node.col, Vec3<U8>(2u));
        if(glm::any(is_less)) {
            /* node.col is guaranteed to be non-zero when is_less is true */
            Vec3<U8> new_color = node.col * (Vec3<U8>)is_less +
                                 map_color * (Vec3<U8>)(glm::not_(is_less));
            chunk.light_lvl[node.idx] = (new_color.r << 11) |
                                         (new_color.g <<  6) |
                                         (new_color.b <<  1);
            if(chunk.wall[node.idx]) {
                node.col = Vec3<U8>(1);
            }
            if(glm::any(atleast_two)) {
                Vec3<U8> side_color = node.col - (Vec3<U8>)atleast_two;
                for(auto const &offset : offsets) {
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
