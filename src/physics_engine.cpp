#include "physics_engine.hpp"

PhysicsEngine::PhysicsEngine() :
    dispatcher(&collision_conf),
    world(&dispatcher, &broadphase, &solver, &collision_conf),
    entity_shape(0.4, 0.9),
    inertia(0, 0, 0)
{
    world.setGravity(btVector3(0.0, 0.0, -9.8));
    entity_shape.calculateLocalInertia(1, inertia);
}

btRigidBody *PhysicsEngine::add_entity(EntityPos const &pos)
{
    motion_states.emplace_back(btTransform({0, 0, 0, 1}, {pos.x, pos.y, pos.z}));
    btRigidBody::btRigidBodyConstructionInfo ci(1, &motion_states.back(),
                                                &entity_shape, btVector3(0, 0, 0));
    ci.m_friction = 2.0;
    bodies.emplace_back(ci);
    world.addRigidBody(&bodies.back());
    return &bodies.back();
}

btRigidBody *PhysicsEngine::add_shape(MapPos const &pos, btCollisionShape *shape)
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
