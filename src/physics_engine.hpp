#pragma once

#include <list>
//
#include <btBulletDynamicsCommon.h>
//
#include <lux/common/entity.hpp>
#include <lux/common/map.hpp>

class PhysicsEngine
{
    public:
    PhysicsEngine();

    btRigidBody *add_entity(entity::Pos const &pos);
    btRigidBody *add_shape(map::Pos const &pos, btCollisionShape *shape);

    void update();
    private:
    std::list<btRigidBody> bodies;
    std::list<btDefaultMotionState> motion_states;

    btDbvtBroadphase                    broadphase;
    btDefaultCollisionConfiguration     collision_conf;
    btCollisionDispatcher               dispatcher;
    btSequentialImpulseConstraintSolver solver;
    btDiscreteDynamicsWorld             world;

    btCapsuleShapeZ entity_shape;
    btVector3 inertia;
};
