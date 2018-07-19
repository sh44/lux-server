#include <world.hpp>
#include <tile/type.hpp>
#include "entity.hpp"

Entity::Entity(World const &world, entity::Type const &type, entity::Pos pos) :
    world(world),
    type(type),
    pos(pos)
{

}

entity::Pos const &Entity::get_pos() const
{
    return pos;
}

void Entity::update()
{

}

void Entity::move(entity::Vec const &by)
{
    auto new_pos = pos + by;
    if(world[new_pos].type->shape != tile::Type::WALL)
    {
        pos = new_pos;
    }
}
