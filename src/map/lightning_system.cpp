#include <glm/glm.hpp>
//
#include <lux/alias/vec_3.hpp>
//
#include <map/chunk.hpp>
#include "lightning_system.hpp"

void LightningSystem::add_node(MapPos const &pos, Vec3UI const &col)
{
    //TODO check for collisions?
    ChkPos chk_pos = to_chk_pos(pos);
    chunk_nodes[chk_pos].emplace(to_chk_idx(pos), col);
    queue_update(chk_pos);
}

void LightningSystem::update(Chunk &chunk, ChkPos const &pos)
{
    if(chunk_nodes.count(pos) == 0) return;

    auto &nodes = chunk_nodes.at(pos);
    while(!nodes.empty()) {
        LightNode node = nodes.front();
        nodes.pop();
        //TODO void_id
        if(chunk.voxels[node.idx] != 0) {
            chunk.light_lvls[node.idx] = 0x0000;
            continue;
        }

        constexpr Vec3I offsets[6] =
            {{-1,  0,  0}, { 1,  0,  0},
             { 0, -1,  0}, { 0,  1,  0},
             { 0,  0, -1}, { 0,  0,  1}};

        MapPos base_pos = to_map_pos(pos, node.idx);
        //TODO bit-level parallelism

        LightLvl map_lvl = chunk.light_lvls[node.idx];
        Vec3UI map_color = {(map_lvl & 0xF000) >> 12,
                            (map_lvl & 0x0F00) >>  8,
                            (map_lvl & 0x00F0) >>  4};
        auto is_less = glm::lessThan(map_color + Vec3UI(1u), node.col);
        auto atleast_two = glm::greaterThanEqual(node.col, Vec3UI(2u));
        if(glm::any(is_less)) {
            /* node.col is guaranteed to be non-zero when is_less is true */
            Vec3UI new_color = node.col * (Vec3UI)is_less +
                               map_color * (Vec3UI)(glm::not_(is_less));
            chunk.light_lvls[node.idx] = (new_color.r << 12) |
                                         (new_color.g <<  8) |
                                         (new_color.b <<  4);
            if(glm::any(atleast_two)) {
                Vec3UI side_color = node.col - 1u * (Vec3UI)atleast_two;
                for(auto const &offset : offsets) {
                    MapPos map_pos = base_pos + offset;
                    ChkPos chk_pos = to_chk_pos(map_pos);
                    ChkIdx idx     = to_chk_idx(map_pos);
                    if(chk_pos == pos) {
                        nodes.emplace(idx, side_color);
                    } else {
                        chunk_nodes[chk_pos].emplace(idx, side_color);
                        queue_update(chk_pos);
                    }
                }
            }
        }
    }
}

void LightningSystem::queue_update(ChkPos const &chk_pos)
{
    update_set.insert(chk_pos);
}
