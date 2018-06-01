#include "entity.hpp"

namespace world::entity
{

Entity::Entity(World const &world, Type const &type, Point pos) :
    world(world),
    type(type),
    pos(pos)
{

}

Point Entity::get_pos() const
{
    return pos;
}

}
