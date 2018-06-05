#include <alias/ref.hpp>
#include <world/entity/common.hpp>
#include "world.hpp"

namespace world
{

World::World(data::Config const &config) :
    config(config),
    map(config)
{

}

Tile &World::operator[](MapPoint pos)
{
    return map[pos];
}

Tile const &World::operator[](MapPoint pos) const
{
    return map[pos];
}

Entity &World::create_player()
{
    return create_entity(Ref<World>(*this), *config.player_type, EntityPoint(0, 0, 0));
}

}
