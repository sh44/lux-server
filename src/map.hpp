#pragma once

#include <lux/alias/hash_map.hpp>
#include <lux/common/map.hpp>
//
#include <map/chunk.hpp>
#include <map/generator.hpp>
#include <physics_engine.hpp>

namespace data { struct Config; class Database; }

class Map
{
    typedef HashMap<ChkPos, map::Chunk>::iterator ChunkIterator;
    public:
    Map(PhysicsEngine &physics_engine, data::Config const &config);
    Map(Map const &that) = delete;
    Map &operator=(Map const &that) = delete;
    ~Map();

    map::TileType const &get_tile(MapPos const &pos);
    map::TileType const &get_tile(MapPos const &pos) const;
    map::Chunk       &get_chunk(ChkPos const &pos);
    map::Chunk const &get_chunk(ChkPos const &pos) const;

    void guarantee_chunk(ChkPos const &pos) const;
    void guarantee_mesh(ChkPos const &pos) const;
    private:
    void try_mesh(ChkPos const &pos) const;
    map::Chunk &load_chunk(ChkPos const &pos) const;
    ChunkIterator unload_chunk(ChunkIterator const &iter) const;

    mutable HashMap<ChkPos, map::Chunk> chunks;
    mutable map::Generator generator;

    data::Database const &db;
    PhysicsEngine &physics_engine;
};
