#pragma once

#include <lux_shared/common.hpp>
#include <lux_shared/entity.hpp>
#include <lux_shared/net/data.hpp>
//
#include <rasen.hpp>

F32 constexpr ENTITY_L_VEL = 0.15f;
F32 constexpr ENTITY_A_VEL = tau / 128.f;

struct Entity {};
extern SparseDynArr<Entity> entities;
typedef decltype(entities)::Id EntityId;

struct EntityComps {
    typedef EntityVec    Pos;
    typedef EntityVec    Vel;
    typedef F32          AVel;
    typedef DynArr<char> Name;
    struct Shape {
        union {
            struct {
                F32 rad;
            } sphere;
            struct {
                Vec2F sz;
            } rect;
        };
        enum Tag : U8 {
            SPHERE,
            RECT,
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
        Vec2F origin;
        F32   angle; ///in radians
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
    IdMap<EntityId, Rasen>       rasen;
};

extern EntityComps& entity_comps;

EntityId create_entity();
EntityId create_item(const char* name);
EntityId create_player();
void remove_entity(EntityId entity);
void entities_tick();
void get_net_entity_comps(NetSsTick::EntityComps* net_comps);
