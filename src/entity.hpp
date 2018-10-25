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
    struct Sphere {
        F32 rad;
    };
    struct Rect {
        Vec2F sz;
    };
    struct Visible {
        U32 visible_id;
    };
    struct Item {
        F32 weight;
    };
    struct Destructible {
        F32 dur;
        F32 dur_max;
    };
    struct Health {
        F32 hp;
        F32 hp_max;
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
    HashTable<EntityId, Sphere>       sphere;
    HashTable<EntityId, Rect>         rect;
    HashTable<EntityId, Visible>      visible;
    HashTable<EntityId, Item>         item;
    HashTable<EntityId, Destructible> destructible;
    HashTable<EntityId, Health>       health;
    HashTable<EntityId, Container>    container;
    HashTable<EntityId, Orientation>  orientation;
};

struct Entity {
    bool deletion_mark = false;
};

extern EntityComps&         entity_comps;

EntityId create_entity();
EntityId create_item(const char* name);
EntityId create_player();
void remove_entity(EntityId entity);
void entities_tick();
void get_net_entity_comps(NetSsTick::EntityComps* net_comps);
