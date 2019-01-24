#pragma once

#include <bullet/btBulletDynamicsCommon.h>
//
#include <lux_shared/common.hpp>
#include <lux_shared/entity.hpp>
#include <lux_shared/map.hpp>

typedef U16 PhysicsBodyId;

void physics_init();
btRigidBody* physics_create_body(EntityVec const& pos);
btRigidBody* physics_create_mesh(MapPos const& pos, btCollisionShape* shape);
void physics_remove_body(btRigidBody* body);
void physics_tick(F32 time);
