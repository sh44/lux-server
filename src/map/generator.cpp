#include <functional>
//
#include <lux/alias/scalar.hpp>
//
#include <data/database.hpp>
#include <data/config.hpp>
#include <map/chunk.hpp>
#include "generator.hpp"

Generator::Generator(PhysicsEngine &physics_engine, data::Config const &config) :
    config(config),
    physics_engine(physics_engine)
{

}

void Generator::generate_chunk(Chunk &chunk, chunk::Pos const &pos)
{
    num_gen.seed(std::hash<chunk::Pos>()(pos));
    for(SizeT i = 0; i < chunk::TILE_SIZE; ++i)
    {
        map::Pos map_pos = chunk::to_map_pos(pos, i);
        if((i % chunk::SIZE.x) % 8 == 0 || ((i / chunk::SIZE.y) % 8) == 0)
        {
            if(num_gen() % 6 >= 5)
            {
                chunk.tiles[i] = Tile(*config.db->tile_types.at("stone_floor"));
                physics_engine.add_empty(map_pos);
            }
            else
            {
                chunk.tiles[i] = Tile(*config.db->tile_types.at("stone_wall"));
                physics_engine.add_block(map_pos);
            }
        }
        else
        {
            chunk.tiles[i] = Tile(*config.db->tile_types.at("stone_floor"));
            physics_engine.add_empty(map_pos);
        }
    }
}
