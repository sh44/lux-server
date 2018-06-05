#pragma once

#include <world/entity/common.hpp>
#include <world/entity/type.hpp>

namespace world
{

class World;

class Entity
{
public:
    Entity(World const &world, EntityType const &type, EntityPoint pos);
    Entity(Entity const &that) = delete;
    Entity &operator=(Entity const &that) = delete;

    EntityPoint get_pos() const;

    World const &world;
    EntityType const &type;
private:
    EntityPoint pos;
};

}
