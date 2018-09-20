#include <lux_shared/containers.hpp>
//
#include <entity.hpp>

List<Entity> entities;

Entity& create_entity(EntityVec const& pos) {
    LUX_LOG("creating new entity");
    LUX_LOG("    pos: {%.2f, %.2f, %.2f}", pos.x, pos.y, pos.z);
    Entity& entity = entities.emplace_front();
    entity.pos = pos;
    return entity;
}

Entity& create_player() {
    LUX_LOG("creating new player");
    return create_entity({0, 0, 0});
}

void entities_tick() {
    for(auto& entity : entities) {
        entity.pos += entity.vel;
        entity.vel += -entity.vel * 0.10f;
    }
}

void remove_entity(Entity& entity) {
    LUX_LOG("unimplemented");
}
