#pragma once

#include <lux/alias/pos_map.hpp>
#include <lux/common/map.hpp>
//
#include <map/chunk.hpp>
#include <map/generator.hpp>
#include <physics_engine.hpp>

namespace data { struct Config; class Database; }

class Map
{
    typedef PosMap<ChkPos, Chunk>::iterator ChunkIterator;
    public:
    Map(PhysicsEngine &physics_engine, data::Config const &config);
    Map(Map const &that) = delete;
    Map &operator=(Map const &that) = delete;
    ~Map();

    VoxelId &get_voxel(MapPos const &pos);
    Chunk &get_chunk(ChkPos const &pos);

    void guarantee_chunk(ChkPos const &pos);
    void guarantee_mesh(ChkPos const &pos);
    private:
    void build_mesh(Chunk &chunk, ChkPos const &pos);
    void update_lightning(Chunk &chunk, ChkPos const &pos);

    Chunk &load_chunk(ChkPos const &pos);
    ChunkIterator unload_chunk(ChunkIterator const &iter);

    PosMap<ChkPos, Chunk> chunks;
    Generator generator;

    data::Database const &db;
    PhysicsEngine &physics_engine;
};
