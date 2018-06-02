#include <alias/int.hpp>
#include <world/map/chunk.hpp>
#include "generator.hpp"

namespace world::map::chunk
{

void Generator::generate_chunk(Chunk &chunk, Point pos)
{
    for(SizeT i = 0; i < Chunk::TILE_SIZE; ++i)
    {
        //new (chunk.tiles + i) Tile(TODO);
    }
}

}
