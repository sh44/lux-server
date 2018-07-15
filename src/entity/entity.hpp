#pragma once

#include <entity/common.hpp>
#include <entity/entity_type.hpp>

class World;

class Entity
{
public:
    Entity(World const &world, EntityType const &type, EntityPoint pos);
    Entity(Entity const &that) = delete;
    Entity &operator=(Entity const &that) = delete;

    EntityPoint const &get_pos() const;

    void update();
    void move(EntityVec const &by);

    World const &world;
    EntityType const &type;

private:

    EntityPoint pos;
};
