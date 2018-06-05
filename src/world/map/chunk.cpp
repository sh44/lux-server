#include <linear/point_2d.hpp>
//
#include "chunk.hpp"

namespace world
{

Chunk::Chunk(Tile *tiles) :
    tiles(tiles)
{

}

ChunkPoint Chunk::point_map_to_chunk(MapPoint point)
{
    ChunkPoint result = point;
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

ChunkIndex Chunk::point_map_to_index(MapPoint point)
{
    point.x %= SIZE.x;
    point.y %= SIZE.y;
    point.z %= SIZE.z;
    if(point.x < 0)
    {
        point.x += SIZE.x;
    }
    if(point.y < 0)
    {
        point.y += SIZE.y;
    }
    if(point.z < 0)
    {
        point.z += SIZE.z;
    }
    linear::Point3d<ChunkIndex> result = point;
    return result.x + (result.y * SIZE.x) + (result.z * SIZE.x * SIZE.y);
}

}
