#pragma once

#include <lux_shared/common.hpp>
#include <lux_shared/entity.hpp>

struct Entity {
    EntityVec pos;
    EntityVec vel = {0, 0, 0};
    U8        flags = 0;
    U32       id;
};

Entity& create_player();
void entities_tick();
void remove_entity(Entity& entity);
