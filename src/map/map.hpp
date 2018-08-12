#pragma once

#include <lux/alias/hash_map.hpp>
#include <lux/common/map.hpp>
//
#include <map/chunk.hpp>
#include <map/generator.hpp>
#include <physics_engine.hpp>

namespace data { struct Config; }

class Map
{
    typedef HashMap<ChkPos, Chunk>::iterator ChunkIterator;
    public:
    Map(PhysicsEngine &physics_engine, data::Config const &config);
    Map(Map const &that) = delete;
    Map &operator=(Map const &that) = delete;
    ~Map();

    Tile        &get_tile(MapPos const &pos);
    Tile  const &get_tile(MapPos const &pos) const;
    Chunk       &get_chunk(ChkPos const &pos);
    Chunk const &get_chunk(ChkPos const &pos) const;

    void guarantee_chunk(ChkPos const &pos) const;
    private:
    Chunk &load_chunk(ChkPos const &pos) const;
    ChunkIterator unload_chunk(ChunkIterator const &iter) const;

    mutable HashMap<ChkPos, Chunk> chunks;
    mutable Generator generator;
};
