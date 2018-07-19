#pragma once

#include <lux/common/entity.hpp>

class World;

namespace entity { struct Type; }

class Entity
{
public:
    Entity(World const &world, entity::Type const &type, entity::Pos pos);
    Entity(Entity const &that) = delete;
    Entity &operator=(Entity const &that) = delete;

    entity::Pos const &get_pos() const;

    void update();
    void move(entity::Vec const &by);

    World const &world;
    entity::Type const &type;

private:

    entity::Pos pos;
};
