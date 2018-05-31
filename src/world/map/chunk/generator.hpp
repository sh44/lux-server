#pragma once

#include <world/map/chunk/point.hpp>

namespace world::map::chunk
{

class Chunk;

class Generator
{
    public:
    void generate_chunk(Chunk &chunk, Point pos);
};

}
