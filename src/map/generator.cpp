#include <functional>
//
#include <lux/alias/scalar.hpp>
//
#include <data/database.hpp>
#include <data/config.hpp>
#include <map/chunk.hpp>
#include "generator.hpp"

Generator::Generator(data::Config const &config) :
    config(config)
{

}

void Generator::generate_chunk(Chunk &chunk, chunk::Pos const &pos)
{
    num_gen.seed(std::hash<chunk::Pos>()(pos));
    for(SizeT i = 0; i < chunk::TILE_SIZE; ++i)
    {
        if((i % chunk::SIZE.x) % 8 == 0 || ((i / chunk::SIZE.y) % 8) == 0)
        {
            if(num_gen() % 6 >= 5)
                chunk.tiles[i] = Tile(*config.db->tile_types.at("stone_floor"));
            else
                chunk.tiles[i] = Tile(*config.db->tile_types.at("stone_wall"));
        }
        else
        {
            chunk.tiles[i] = Tile(*config.db->tile_types.at("stone_floor"));
        }
    }
}
