#pragma once

#include <alias/hash_map.hpp>
#include <map/common.hpp>
#include <map/chunk/common.hpp>
#include <map/chunk/chunk.hpp>
#include <map/generator.hpp>

namespace data { struct Config; }

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
