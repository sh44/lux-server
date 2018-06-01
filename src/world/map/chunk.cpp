#include <linear/point_2d.hpp>
//
#include "chunk.hpp"

namespace world::map::chunk
{

Chunk::Chunk(Tile *tiles) :
    tiles(tiles)
{

}

Point Chunk::point_map_to_chunk(map::Point point)
{
    chunk::Point result = point;
    if(result.x < 0)
    {
        result.x -= SIZE.x - 1;
    }
    if(result.y < 0)
    {
        result.y -= SIZE.y - 1;
    }
    if(result.z < 0)
    {
        result.z -= SIZE.z - 1;
    }
    result.x /= SIZE.x;
    result.y /= SIZE.y;
    result.z /= SIZE.z;
    return result;
}

Index Chunk::point_map_to_index(map::Point point)
{
    linear::Point3d<Index> result = point;
    result.x %= SIZE.x;
    result.y %= SIZE.y;
    result.z %= SIZE.z;
    if(result.x < 0)
    {
        result.x += SIZE.x;
    }
    if(result.y < 0)
    {
        result.y += SIZE.y;
    }
    if(result.z < 0)
    {
        result.z += SIZE.z;
    }
    return result.x + (result.y * SIZE.x) + (result.z * SIZE.x * SIZE.y);

}

}
