#include <functional>
//
#include <alias/int.hpp>
#include <data/database.hpp>
#include <data/config.hpp>
#include <map/chunk/chunk.hpp>
#include "generator.hpp"

Generator::Generator(data::Config const &config) :
    config(config)
{

}

void Generator::generate_chunk(Chunk &chunk, ChunkPoint const &pos)
{
    num_gen.seed(std::hash<ChunkPoint>()(pos));
    for(SizeT i = 0; i < Chunk::TILE_SIZE; ++i)
    {
        if((i % Chunk::SIZE.x) % 8 == 0 || ((i / Chunk::SIZE.y) % 8) == 0)
        {
            if(num_gen() % 6 >= 5)
                new (chunk.tiles + i) Tile(*config.db->tile_types.at("stone_floor"));
            else
                new (chunk.tiles + i) Tile(*config.db->tile_types.at("stone_wall"));
        }
        else
        {
            new (chunk.tiles + i) Tile(*config.db->tile_types.at("stone_floor"));
        }
    }
}
