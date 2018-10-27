#pragma once

#include <lux_shared/common.hpp>
#include <lux_shared/entity.hpp>
#include <lux_shared/net/data.hpp>

struct Entity;
extern SparseDynArr<Entity> entities;
typedef decltype(entities)::Id EntityId;

struct EntityComps {
    typedef EntityVec    Pos;
    typedef EntityVec    Vel;
    typedef DynArr<char> Name;
    struct Shape {
        union {
            struct {
                F32 len;
            } line;
            struct {
                F32 rad;
            } sphere;
            struct {
                Vec2F sz;
            } rect;
        };
        enum Tag : U8 {
            POINT,
            LINE,
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
        F32 angle; ///in radians
    };

    //@IMPROVE more space efficient solution
    HashTable<EntityId, Pos>          pos;
    HashTable<EntityId, Vel>          vel;
    HashTable<EntityId, Name>         name;
    HashTable<EntityId, Shape>        shape;
    HashTable<EntityId, Visible>      visible;
    HashTable<EntityId, Item>         item;
    HashTable<EntityId, Container>    container;
    HashTable<EntityId, Orientation>  orientation;
};

struct Entity {
    bool deletion_mark = false;
};

extern EntityComps&         entity_comps;

void entity_rotate_to(EntityId id, F32 angle);
EntityId create_entity();
EntityId create_item(const char* name);
EntityId create_player();
void remove_entity(EntityId entity);
void entities_tick();
void get_net_entity_comps(NetSsTick::EntityComps* net_comps);
