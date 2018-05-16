#pragma once

#include <alias/hash_map.hpp>
#include <world/map/point.hpp>
#include <world/map/chunk.hpp>

namespace world
{

class Map
{
    private:
    HashMap<map::Point, map::Chunk> chunks;
};

}
