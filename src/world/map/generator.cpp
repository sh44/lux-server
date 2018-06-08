#include <functional>
//
#include <alias/int.hpp>
#include <data/database.hpp>
#include <data/config.hpp>
#include <world/map/chunk.hpp>
#include "generator.hpp"

namespace world
{

Generator::Generator(data::Config const &config) :
    config(config)
{

}

void Generator::generate_chunk(Chunk &chunk, ChunkPoint pos)
{
    num_gen.seed(std::hash<ChunkPoint>()(pos));
    for(SizeT i = 0; i < Chunk::TILE_SIZE; ++i)
    {
        if(num_gen() % 10 == 0)
        {
            new (chunk.tiles + i) Tile(*config.db->tile_types.at("void"));
        }
        else
        {
            new (chunk.tiles + i) Tile(*config.db->tile_types.at("grass"));
        }
    }
}

}
