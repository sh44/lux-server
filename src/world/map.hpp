#pragma once

#include <alias/hash_map.hpp>
#include <world/map/point.hpp>
#include <world/map/chunk.hpp>
#include <world/map/chunk/generator.hpp>

namespace world
{
inline namespace map
{

class Map
{
    public:
    Tile       &operator[](Point pos);
    Tile const &operator[](Point pos) const;
    private:
    Chunk &load_chunk(chunk::Point pos) const;
    void unload_chunk(chunk::Point pos) const;
    Chunk &get_chunk(chunk::Point pos)  const;

    mutable HashMap<chunk::Point, Chunk> chunks;
    mutable chunk::Generator generator;
};

}
}
