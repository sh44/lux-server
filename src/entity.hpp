#pragma once

#include <lux_shared/common.hpp>
#include <lux_shared/entity.hpp>
#include <lux_shared/net/data.hpp>
//
#include <physics.hpp>

F32 constexpr ENTITY_L_VEL = 40.f;

struct Entity {};
extern SparseDynArr<Entity> entities; //@TODO

struct EntityComps {
    typedef StrBuff      Name;
    typedef btRigidBody* PhysicsBody;
    struct Model {
        U32 id;
    };

    //@IMPROVE more space efficient solution
    IdMap<EntityId, Name>        name;
    IdMap<EntityId, PhysicsBody> physics_body;
    IdMap<EntityId, Model>       model;
};

extern EntityComps& entity_comps;

EntityId create_entity();
EntityId create_player();
void entity_erase(EntityId entity);
void entities_tick();
void get_net_entity_comps(NetSsTick::EntityComps* net_comps);
void entity_set_vel(EntityId id, EntityVec vel);
void entity_rotate_yaw(EntityId id, F32 yaw);
void entity_rotate_yaw_pitch(EntityId id, Vec2F yaw_pitch);
