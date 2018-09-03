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

        LightLvl light_lvl =
            chunk.light_lvls[to_chk_idx(node.pos)];
        MapPos base_pos = to_map_pos(pos, node.pos);
        nodes.pop();
        if(light_lvl > 1) {
            for(auto const &offset : offsets) {
                MapPos map_pos = base_pos + offset;
                if(to_chk_pos(map_pos) == pos) {
                    ChkIdx idx = to_chk_idx(map_pos);
                    //TODO void_id
                    LightLvl side_lvl = (chunk.light_lvls[idx] & 0xF000) >> 12;
                    if(side_lvl <= ((light_lvl & 0xF000) >> 12) - 2) {
                        chunk.light_lvls[idx] &= 0x0FFF;
                        chunk.light_lvls[idx] |= (light_lvl & 0xF000) - 0x1000;
                        nodes.emplace(to_idx_pos(map_pos));
                    }
                }
            }
        }
    }
}
