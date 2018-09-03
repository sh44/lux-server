#pragma once

#include <lux/alias/queue.hpp>
#include <lux/alias/pos_map.hpp>
#include <lux/common/map.hpp>

class LightningSystem
{
    struct LightNode
    {
        IdxPos   pos;
        LightNode(IdxPos const &pos) : pos(pos) { }
    };
    public:
    void add_node(MapPos const &pos);
    void update(Chunk &chunk, ChkPos const &pos);
    private:
    PosMap<ChkPos, Queue<LightNode>> chunk_nodes;
};
