#pragma once

#include <btBulletDynamicsCommon.h>
//
#include <lux/common/entity.hpp>

class World;

namespace entity { struct Type; }

class Entity
{
public:
    Entity(World const &world, entity::Type const &type, btRigidBody *body);
    Entity(Entity const &that) = delete;
    Entity &operator=(Entity const &that) = delete;

    entity::Pos get_pos() const;

    void update();
    void move(entity::Vec const &by);

    World const &world;
    entity::Type const &type;
    bool deletion_mark;
private:
    btRigidBody *body;
};
