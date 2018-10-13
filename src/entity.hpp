#pragma once

#include <lux_shared/common.hpp>
#include <lux_shared/entity.hpp>

typedef U32 EntityHandle;

struct EntityComponents {
    typedef EntityVec Pos;
    typedef EntityVec Vel;
    struct Shape {
        F32 rad;
    };

    HashTable<EntityHandle, Pos>     pos;
    HashTable<EntityHandle, Vel>     vel;
    HashTable<EntityHandle, Shape> shape;
};

extern EntityComponents& entity_comps;
EntityHandle create_player();
void entities_tick();
void remove_entity(EntityHandle entity);
