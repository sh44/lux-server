#pragma once

#include <list>
//
#include <btBulletDynamicsCommon.h>
//
#include <lux/common.hpp>
#include <lux/world/entity.hpp>
#include <lux/world/map.hpp>

class PhysicsEngine
{
    public:
    PhysicsEngine();
    LUX_NO_COPY(PhysicsEngine);

    btRigidBody *add_entity(EntityPos const &pos);
    btRigidBody *add_shape(MapPos const &pos, btCollisionShape *shape);

    void tick();
    private:
    std::list<btRigidBody> bodies;
    std::list<btDefaultMotionState> motion_states; //TODO is this needed?

    btDbvtBroadphase                    broadphase;
    btDefaultCollisionConfiguration     collision_conf;
    btCollisionDispatcher               dispatcher;
    btSequentialImpulseConstraintSolver solver;
    btDiscreteDynamicsWorld             world;

    btCapsuleShapeZ entity_shape;
    btVector3 inertia; //TODO is this needed?
};
