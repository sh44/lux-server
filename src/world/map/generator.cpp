#include <world/map/chunk/index.hpp>
#include <world/map/chunk.hpp>
//
#include "generator.hpp"

namespace world::map
{

void Generator::generate_chunk(Chunk &chunk, chunk::Point pos)
{
    for(chunk::Index i = 0; i < Chunk::SIZE_X * Chunk::SIZE_Y * Chunk::SIZE_Z; ++i)
    {
        chunk.tiles[i] = Tile();
    }
}

}
