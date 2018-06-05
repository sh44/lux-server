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
    public:
    Map(data::Config const &config);
    ~Map();

    Tile       &operator[](MapPoint pos);
    Tile const &operator[](MapPoint pos) const;
    private:
    Chunk &load_chunk(ChunkPoint pos) const;
    void unload_chunk(ChunkPoint pos) const;
    Chunk &get_chunk(ChunkPoint pos)  const;

    mutable HashMap<ChunkPoint, Chunk> chunks;
    mutable Generator generator;
};

}
