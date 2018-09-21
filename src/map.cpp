#include <lux_shared/common.hpp>
#include <lux_shared/util/packer.hpp>
#include <lux_shared/vec_hash.hpp>
#include <lux_shared/map.hpp>
//
#include <db.hpp>
#include "map.hpp"

VecMap<ChkPos, Chunk> chunks;

struct LightNode {
    ChkIdx   idx;
    Vec3<U8> col;
};
VecSet<ChkPos> lightning_updates;
VecMap<ChkPos, Queue<LightNode>> lightning_nodes;

static void update_lightning(ChkPos const &pos);
static bool is_chunk_loaded(ChkPos const& pos) {
    return chunks.count(pos) > 0;
}

void map_tick(DynArr<ChkPos>& light_updated_chunks) {
    ///we need to copy it, because update_lightning could add another updates
    auto l_updates = lightning_updates;
    for(auto it = l_updates.begin(); it != l_updates.end(); ++it) {
        if(is_chunk_loaded(*it)) {
            update_lightning(*it);
            lightning_updates.erase(*it);
            light_updated_chunks.push_back(*it);
        };
    }
}

static Chunk& load_chunk(ChkPos const& pos) {
    LUX_ASSERT(!is_chunk_loaded(pos));
    LUX_LOG("loading chunk");
    LUX_LOG("    pos: {%zd, %zd, %zd}", pos.x, pos.y, pos.z);
    ///@RESEARCH to do a better way to no-copy default construct
    Chunk& chunk = chunks[pos];
    for(Uns i = 0; i < CHK_VOL; ++i) {
        VoxelId voxel_id = db_voxel_id("void");
        MapPos map_pos = to_map_pos(pos, i);
        Uns pos_hash = std::hash<MapPos>()(map_pos);
        if((map_pos.x % 8 == 0 || map_pos.y % 8 == 0) && pos_hash % 5 != 0) {
            voxel_id = db_voxel_id("stone_wall");
        } else {
            /*if(pos_hash % 13 == 0) {
                voxel_id = db_voxel_id("void");
            } else */{
                voxel_id = db_voxel_id("stone_floor");
            }
        }
        if((map_pos.x % 8 == 3 && map_pos.y % 8 == 3) && pos_hash % 20 == 0) {
            add_light_node(to_map_pos(pos, i), {0xF, 0xF, 0xF});
        }
        chunk.voxels[i] = voxel_id;
        chunk.light_lvls[i] = 0;
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

VoxelId get_voxel(MapPos const& pos) {
    return get_chunk(to_chk_pos(pos)).voxels[to_chk_idx(pos)];
}

VoxelType const& get_voxel_type(MapPos const& pos) {
    return db_voxel_type(get_voxel(pos));
}

void add_light_node(MapPos const& pos, Vec3<U8> col) {
    ChkPos chk_pos = to_chk_pos(pos);
    lightning_nodes[chk_pos].push({to_chk_idx(pos), col});
    lightning_updates.insert(chk_pos);
}

static void update_lightning(ChkPos const &pos)
{
    if(lightning_nodes.count(pos) == 0) return;

    Chunk& chunk = chunks.at(pos);
    auto &nodes = lightning_nodes.at(pos);
    while(!nodes.empty()) {
        LightNode node = nodes.front();
        nodes.pop();

        if(db_voxel_type(chunk.voxels[node.idx]).shape == VoxelType::BLOCK) {
            chunk.light_lvls[node.idx] = 0x0000;
            continue;
        }

        constexpr Vec3I offsets[6] =
            {{-1,  0,  0}, { 1,  0,  0},
             { 0, -1,  0}, { 0,  1,  0}};

        MapPos base_pos = to_map_pos(pos, node.idx);
        //TODO bit-level parallelism

        LightLvl map_lvl = chunk.light_lvls[node.idx];
        Vec3<U8> map_color = {(map_lvl & 0xF000) >> 12,
                              (map_lvl & 0x0F00) >>  8,
                              (map_lvl & 0x00F0) >>  4};
        auto is_less = glm::lessThan(map_color, node.col);
        auto atleast_two = glm::greaterThanEqual(node.col, Vec3<U8>(2u));
        if(glm::any(is_less)) {
            /* node.col is guaranteed to be non-zero when is_less is true */
            Vec3<U8> new_color = node.col * (Vec3<U8>)is_less +
                                 map_color * (Vec3<U8>)(glm::not_(is_less));
            chunk.light_lvls[node.idx] = (new_color.r << 12) |
                                         (new_color.g <<  8) |
                                         (new_color.b <<  4);
            if(glm::any(atleast_two)) {
                Vec3<U8> side_color = node.col - (Vec3<U8>)atleast_two;
                for(auto const &offset : offsets) {
                    //@TODO don't spread lights through Z if there is floor
                    MapPos map_pos = base_pos + offset;
                    ChkPos chk_pos = to_chk_pos(map_pos);
                    ChkIdx idx     = to_chk_idx(map_pos);
                    if(chk_pos == pos) {
                        nodes.push({idx, side_color});
                    } else {
                        lightning_nodes[chk_pos].push({idx, side_color});
                        lightning_updates.insert(chk_pos);
                    }
                }
            }
        }
    }
}
