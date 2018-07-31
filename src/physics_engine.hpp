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

    btBoxShape    block_shape;
    btEmptyShape  empty_shape;
    btSphereShape sphere_shape;

    btDefaultMotionState motion_state;
    btRigidBody::btRigidBodyConstructionInfo block_ci;
    btRigidBody::btRigidBodyConstructionInfo empty_ci;
    btRigidBody::btRigidBodyConstructionInfo sphere_ci;

    btVector3 inertia;
};
