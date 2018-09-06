#pragma once

#include <lux/alias/pos_map.hpp>
#include <lux/alias/queue.hpp>
#include <lux/world/map.hpp>
//
#include <map/chunk.hpp>
#include <map/generator.hpp>
#include <map/lightning_system.hpp>
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

    Chunk const &get_chunk(ChkPos const &pos);

    void guarantee_chunk(ChkPos const &pos);
    void guarantee_mesh(ChkPos const &pos);

    void tick();
    private:
    void build_mesh(Chunk &chunk, ChkPos const &pos);
    void lightning_tick();

    Chunk &load_chunk(ChkPos const &pos);
    ChunkIterator unload_chunk(ChunkIterator const &iter);

    PosMap<ChkPos, Chunk> chunks;
    Generator generator;
    LightningSystem lightning_system;

    data::Database const &db;
    PhysicsEngine &physics_engine;
};
