#pragma once

#include <lux/alias/queue.hpp>
#include <lux/alias/set.hpp>
#include <lux/alias/pos_map.hpp>
#include <lux/common/map.hpp>

class LightningSystem
{
    struct LightNode
    {
        IdxPos pos;
        Vec3UI col;
        LightNode(IdxPos const &pos, Vec3UI const &col) :
            pos(pos), col(col) { }
    };
    public:
    void add_node(MapPos const &pos, Vec3UI const &col);
    void update(Chunk &chunk, ChkPos const &pos);

    Set<ChkPos> update_set;
    private:
    void queue_update(ChkPos const &pos);
    PosMap<ChkPos, Queue<LightNode>> chunk_nodes;
};
