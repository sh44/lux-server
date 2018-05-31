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
        result.x -= SIZE_X - 1;
    }
    if(result.y < 0)
    {
        result.y -= SIZE_Y - 1;
    }
    if(result.z < 0)
    {
        result.z -= SIZE_Z - 1;
    }
    result.x /= SIZE_X;
    result.y /= SIZE_Y;
    result.z /= SIZE_Z;
    return result;
}

Index Chunk::point_map_to_index(map::Point point)
{
    linear::Point3d<Index> result = point;
    result.x %= SIZE_X;
    result.y %= SIZE_Y;
    result.z %= SIZE_Z;
    if(result.x < 0)
    {
        result.x += SIZE_X;
    }
    if(result.y < 0)
    {
        result.y += SIZE_Y;
    }
    if(result.z < 0)
    {
        result.z += SIZE_Z;
    }
    return result.x + (result.y * SIZE_X) + (result.z * SIZE_X * SIZE_Y);

}

}
