#pragma once

#include <lux/alias/hash_map.hpp>
#include <lux/common/map.hpp>
#include <lux/common/chunk.hpp>
//
#include <map/chunk.hpp>
#include <map/generator.hpp>
#include <physics_engine.hpp>

namespace data { struct Config; }

class Map
{
    typedef HashMap<chunk::Pos, Chunk>::iterator ChunkIterator;
    public:
    Map(PhysicsEngine &physics_engine, data::Config const &config);
    Map(Map const &that) = delete;
    Map &operator=(Map const &that) = delete;
    ~Map();

    Tile        &operator[](map::Pos const &pos);
    Tile  const &operator[](map::Pos const &pos) const;
    Chunk       &operator[](chunk::Pos const &pos);
    Chunk const &operator[](chunk::Pos const &pos) const;

    void guarantee_chunk(chunk::Pos const &pos) const;
    private:
    Chunk &load_chunk(chunk::Pos const &pos) const;
    ChunkIterator unload_chunk(ChunkIterator const &iter) const;

    mutable HashMap<chunk::Pos, Chunk> chunks;
    mutable Generator generator;
};
