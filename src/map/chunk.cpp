#include <lux/consts.hpp>
//
#include "chunk.hpp"

const ChunkSize &Chunk::SIZE      = consts::CHUNK_SIZE;
const SizeT     &Chunk::TILE_SIZE = consts::CHUNK_TILE_SIZE;

Chunk::Chunk(Tile *tiles) :
    tiles(tiles)
{

}

ChunkPos Chunk::pos_map_to_chunk(MapPos const &pos)
{
    ChunkPos result = pos;
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

ChunkIndex Chunk::pos_map_to_index(MapPos const &pos)
{
    MapPos result = pos;
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
    return (ChunkIndex)(result.x + (result.y * SIZE.x) + (result.z * SIZE.x * SIZE.y));
}
