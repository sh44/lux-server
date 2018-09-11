#pragma once

#include <btBulletDynamicsCommon.h>
//
#include <lux/common.hpp>
#include <lux/world/entity.hpp>

class World;

struct EntityType;

class Entity
{
public:
    Entity(World &world, EntityType const &type, btRigidBody *body);
    LUX_NO_COPY(Entity);

    EntityPos get_pos() const;

    void tick();
    void move(EntityVec const &by);
    void jump();

    World &world;
    EntityType const &type;
    bool deletion_mark;
private:
    btRigidBody *body;
};
