#include "physics_engine.hpp"

PhysicsEngine::PhysicsEngine() :
    dispatcher(&collision_conf),
    world(&dispatcher, &broadphase, &solver, &collision_conf),
    block_shape(btVector3(0.5, 0.5, 0.5)),
    sphere_shape(0.2),
    motion_state(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 0, 0))),
    block_ci(0, &motion_state, &block_shape, btVector3(0, 0, 0)),
    empty_ci(0, &motion_state, &empty_shape, btVector3(0, 0, 0)),
    sphere_ci(1, &motion_state, &sphere_shape, btVector3(0, 0, 0)),
    inertia(0, 0, 0)
{
    world.setGravity(btVector3(0, 0, -9.8));
    sphere_shape.calculateLocalInertia(1, inertia);
}

btRigidBody *PhysicsEngine::add_entity(entity::Pos const &pos)
{
    motion_states.emplace_back(btTransform({0, 0, 0, 1}, {pos.x, pos.y, pos.z}));
    sphere_ci.m_motionState = &motion_states.back();
    bodies.emplace_back(sphere_ci);
    world.addRigidBody(&bodies.back());
    return &bodies.back();
}

btRigidBody *PhysicsEngine::add_shape(map::Pos const &pos, btCollisionShape *shape)
{
    motion_states.emplace_back(btTransform({0, 0, 0, 1},
        {(F32)pos.x, (F32)pos.y, (F32)pos.z}));
    btRigidBody::btRigidBodyConstructionInfo ci(0, &motion_states.back(),
                                                shape, btVector3(0, 0, 0));
    bodies.emplace_back(ci);
    world.addRigidBody(&bodies.back());
    return &bodies.back();
}

void PhysicsEngine::update()
{
    world.stepSimulation(1.0f/64.f, 1, 1.0f/60.f);
}
