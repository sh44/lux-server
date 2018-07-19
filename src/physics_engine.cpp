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
    bodies.emplace_back(sphere_ci);
    world.addRigidBody(&bodies.back());
    bodies.back().translate(btVector3(pos.x, pos.y, pos.z));
    return &bodies.back();
}

btRigidBody *PhysicsEngine::add_block(map::Pos const &pos)
{
    bodies.emplace_back(block_ci);
    world.addRigidBody(&bodies.back());
    bodies.back().translate(btVector3(pos.x, pos.y, pos.z));
    return &bodies.back();
}

btRigidBody *PhysicsEngine::add_empty(map::Pos const &pos)
{
    bodies.emplace_back(empty_ci);
    world.addRigidBody(&bodies.back());
    bodies.back().translate(btVector3(pos.x, pos.y, pos.z));
    return &bodies.back();
}

void PhysicsEngine::update()
{
    world.stepSimulation(1.0f/64.f, 1, 1.0f/60.f);
}
