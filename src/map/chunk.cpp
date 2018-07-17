#include <lux/consts.hpp>
//
#include "chunk.hpp"

const ChunkSize &Chunk::SIZE      = consts::CHUNK_SIZE;
const SizeT     &Chunk::TILE_SIZE = consts::CHUNK_TILE_SIZE;

Chunk::Chunk(Tile *tiles) :
    tiles(tiles)
{

}

