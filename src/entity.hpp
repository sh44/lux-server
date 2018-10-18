#pragma once

#include <lux_shared/common.hpp>
#include <lux_shared/entity.hpp>

extern EntityComps& entity_comps;
EntityHandle create_entity(EntityVec const& pos);
EntityHandle create_player();
void entities_tick();
void remove_entity(EntityHandle entity);
