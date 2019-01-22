#pragma once

#include <lux_shared/common.hpp>
#include <lux_shared/entity.hpp>
#include <lux_shared/net/data.hpp>
//
#include <rasen.hpp>

F32 constexpr ENTITY_L_VEL = 0.015f;
F32 constexpr ENTITY_A_VEL = tau / 512.f;

struct Entity {};
extern SparseDynArr<Entity> entities;

struct EntityComps {
    typedef EntityVec Pos;
    typedef EntityVec Vel;
    typedef StrBuff   Name;
    struct Shape {
        union {
            struct {
                F32 rad;
            } sphere;
            struct {
                Vec3F sz;
            } box;
        };
        enum Tag : U8 {
            SPHERE,
            BOX,
        } tag;
    };
    struct Visible {
        U32 visible_id;
    };
    struct Item {
        F32 weight;
    };
    struct Container {
        DynArr<EntityId> items;
    };
    struct Orientation {
        EntityVec origin;
        Vec2F     angles; ///pitch, yaw, in radians
    };
    struct AVel {
        Vec2F vel;
        Vec2F damping; ///from 0.f to 1.f
    };
    struct Ai {
        RasenEnv rn_env;
        bool active = true;
    };

    //@IMPROVE more space efficient solution
    IdMap<EntityId, Pos>         pos;
    IdMap<EntityId, Vel>         vel;
    IdMap<EntityId, AVel>        a_vel;
    IdMap<EntityId, Name>        name;
    IdMap<EntityId, Shape>       shape;
    IdMap<EntityId, Visible>     visible;
    IdMap<EntityId, Item>        item;
    IdMap<EntityId, Container>   container;
    IdMap<EntityId, Orientation> orientation;
    IdMap<EntityId, EntityId>    parent;
    IdMap<EntityId, Ai>          ai;
};

extern EntityComps& entity_comps;

EntityId create_entity();
EntityId create_item(Str name);
EntityId create_player();
void remove_entity(EntityId entity);
void entities_tick();
void get_net_entity_comps(NetSsTick::EntityComps* net_comps);
LUX_MAY_FAIL entity_do_action(EntityId entity_id, U16 action_id,
                              Slice<U8> const& stack);
