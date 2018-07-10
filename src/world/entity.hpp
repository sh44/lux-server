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

    EntityPoint const &get_pos() const;

    void update();

    World const &world;
    EntityType const &type;
private:
    void move(EntityVector const &by);

    EntityPoint pos;
};

}
