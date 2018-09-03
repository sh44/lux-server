#include <glm/glm.hpp>
//
#include <lux/alias/vec_3.hpp>
//
#include <map/chunk.hpp>
#include "lightning_system.hpp"

void LightningSystem::add_node(MapPos const &pos)
{
    //TODO check for collisions?
    chunk_nodes[to_chk_pos(pos)].emplace(to_idx_pos(pos));
}

void LightningSystem::update(Chunk &chunk, ChkPos const &pos)
{
    if(chunk_nodes.count(pos) == 0) return;

    auto &nodes = chunk_nodes.at(pos);
    while(!nodes.empty()) {
        LightNode node = nodes.front();
        constexpr MapPos offsets[6] =
            {{-1,  0,  0}, { 1,  0,  0},
             { 0, -1,  0}, { 0,  1,  0},
             { 0,  0, -1}, { 0,  0,  1}};

        LightLvl light_lvl = chunk.light_lvls[to_chk_idx(node.pos)];
        MapPos base_pos = to_map_pos(pos, node.pos);
        nodes.pop();
        Vec3UI color = {(light_lvl & 0xF000) >> 12,
                        (light_lvl & 0x0F00) >>  8,
                        (light_lvl & 0x00F0) >>  4};
        //TODO is there a performance hit if any color is below 2?
        //TODO bit-level parallelism
        //TODO outside chunk support
        for(auto const &offset : offsets) {
            MapPos map_pos = base_pos + offset;
            if(to_chk_pos(map_pos) == pos) {
                ChkIdx idx = to_chk_idx(map_pos);
                //TODO void_id
                LightLvl side_lvl = chunk.light_lvls[idx];
                Vec3UI side_color = {(side_lvl & 0xF000) >> 12,
                                     (side_lvl & 0x0F00) >>  8,
                                     (side_lvl & 0x00F0) >>  4};
                Vec3UI new_color;
                auto atleast_two = glm::greaterThanEqual(color, Vec3UI(2u));
                auto is_less = glm::lessThanEqual(side_color, color - 2u) *
                               atleast_two;
                new_color = color - 1u * (Vec3UI)is_less;
                if(glm::any(is_less)) {
                    chunk.light_lvls[idx] = (new_color.r << 12) |
                                            (new_color.g <<  8) |
                                            (new_color.b <<  4);
                    nodes.emplace(to_idx_pos(map_pos));
                }
            }
        }
    }
}
