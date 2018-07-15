#include "entity.hpp"

Entity::Entity(World const &world, EntityType const &type, EntityPoint pos) :
    world(world),
    type(type),
    pos(pos)
{

}

EntityPoint const &Entity::get_pos() const
{
    return pos;
}

void Entity::update()
{

}

void Entity::move(EntityVec const &by)
{
    pos += by;
}
