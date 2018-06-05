#include "entity.hpp"

namespace world
{

Entity::Entity(World const &world, EntityType const &type, EntityPoint pos) :
    world(world),
    type(type),
    pos(pos)
{

}

EntityPoint Entity::get_pos() const
{
    return pos;
}

}
