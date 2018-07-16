#pragma once

#include <alias/hash_map.hpp>
#include <common/map.hpp>
#include <common/chunk.hpp>
#include <map/chunk.hpp>
#include <map/generator.hpp>

namespace data { struct Config; }

class Map
{
    typedef HashMap<ChunkPos, Chunk>::iterator ChunkIterator;
    public:
    Map(data::Config const &config);
    Map(Map const &that) = delete;
    Map &operator=(Map const &that) = delete;
    ~Map();

    Tile       &operator[](MapPos const &pos);
    Tile const &operator[](MapPos const &pos) const;
    private:
    Chunk &load_chunk(ChunkPos const &pos) const;
    ChunkIterator unload_chunk(ChunkIterator const &iter) const;
    Chunk &get_chunk(ChunkPos const &pos)  const;

    mutable HashMap<ChunkPos, Chunk> chunks;
    mutable Generator generator;
};
