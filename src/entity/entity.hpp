#pragma once

#include <btBulletDynamicsCommon.h>
//
#include <lux/common/entity.hpp>

class World;

struct EntityType;

class Entity
{
public:
    Entity(World const &world, EntityType const &type, btRigidBody *body);
    Entity(Entity const &that) = delete;
    Entity &operator=(Entity const &that) = delete;

    EntityPos get_pos() const;

    void update();
    void move(EntityVec const &by);
    void jump();

    World const &world;
    EntityType const &type;
    bool deletion_mark;
private:
    btRigidBody *body;
};
