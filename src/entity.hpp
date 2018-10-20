#pragma once

#include <lux_shared/common.hpp>
#include <lux_shared/entity.hpp>
#include <lux_shared/net/data.hpp>

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
        DynArr<EntityHandle> items;
    };
    struct Orientation {
        F32 angle; ///in radians
    };

    //@IMPROVE more space efficient solution
    HashTable<EntityHandle, Pos>          pos;
    HashTable<EntityHandle, Vel>          vel;
    HashTable<EntityHandle, Name>         name;
    HashTable<EntityHandle, Sphere>       sphere;
    HashTable<EntityHandle, Rect>         rect;
    HashTable<EntityHandle, Visible>      visible;
    HashTable<EntityHandle, Item>         item;
    HashTable<EntityHandle, Destructible> destructible;
    HashTable<EntityHandle, Health>       health;
    HashTable<EntityHandle, Container>    container;
    HashTable<EntityHandle, Orientation>  orientation;
};

struct Entity {
    bool deletion_mark = false;
};

extern SparseDynArr<Entity> entities;
extern EntityComps&         entity_comps;

EntityHandle create_entity();
EntityHandle create_player();
void remove_entity(EntityHandle entity);
void entities_tick();
void get_net_entity_comps(NetSsTick::EntityComps* net_comps);
