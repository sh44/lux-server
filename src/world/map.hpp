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
    Tile &operator[](Point pos);
    private:
    Chunk &load_chunk(chunk::Point pos);
    void unload_chunk(chunk::Point pos);
    Chunk &get_chunk(chunk::Point pos);

    HashMap<chunk::Point, Chunk> chunks;
    chunk::Generator generator;
};

}
}
