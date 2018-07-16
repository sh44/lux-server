#include <world.hpp>
#include <tile/tile_type.hpp>
#include "entity.hpp"

Entity::Entity(World const &world, EntityType const &type, EntityPos pos) :
    world(world),
    type(type),
    pos(pos)
{

}

EntityPos const &Entity::get_pos() const
{
    return pos;
}

void Entity::update()
{

}

void Entity::move(EntityVec const &by)
{
    auto new_pos = pos + by;
    if(world[new_pos].type->shape != TileType::WALL)
    {
        pos = new_pos;
    }
}
