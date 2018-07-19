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
    btRigidBody *add_block(map::Pos const &pos);
    btRigidBody *add_empty(map::Pos const &pos);

    void update();
    private:
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

    std::list<btRigidBody> bodies;
};
