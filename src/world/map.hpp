#pragma once

#include <alias/hash_map.hpp>
#include <world/map/common.hpp>
#include <world/map/chunk/common.hpp>
#include <world/map/chunk.hpp>
#include <world/map/generator.hpp>

namespace data { struct Config; }

namespace world
{

class Map
{
    typedef HashMap<ChunkPoint, Chunk>::iterator ChunkIterator;
    public:
    Map(data::Config const &config);
    Map(Map const &that) = delete;
    Map &operator=(Map const &that) = delete;
    ~Map();

    Tile       &operator[](MapPoint const &pos);
    Tile const &operator[](MapPoint const &pos) const;
    private:
    Chunk &load_chunk(ChunkPoint const &pos) const;
    ChunkIterator unload_chunk(ChunkIterator const &iter) const;
    Chunk &get_chunk(ChunkPoint const &pos)  const;

    mutable HashMap<ChunkPoint, Chunk> chunks;
    mutable Generator generator;
};

}
