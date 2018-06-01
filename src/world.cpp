#include <world/entity/point.hpp>
#include "world.hpp"

namespace world
{

World::World(data::Config const &config) :
    config(config)
{

}

Tile &World::operator[](map::Point pos)
{
    return map[pos];
}

Tile const &World::operator[](map::Point pos) const
{
    return map[pos];
}

Entity &World::create_player()
{
    return create_entity(*this, *config.player_type, entity::Point(0, 0, 0));
}

}
