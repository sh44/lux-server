#pragma once

#include <world/map/chunk/point.hpp>

namespace world::map
{

namespace chunk { class Chunk; }

class Generator
{
    public:
    void generate_chunk(Chunk &chunk, chunk::Point pos);
    private:
};

}
