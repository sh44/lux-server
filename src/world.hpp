#pragma once

#include <list>
//
#include <lux/alias/vector.hpp>
#include <lux/common.hpp>
//
#include <data/database.hpp>
#include <data/config.hpp>
#include <entity/entity_type.hpp>
#include <entity/entity.hpp>
#include <map/map.hpp>

class World : public Map
{
public:
    World(data::Config const &config);
    World(World const &that) = delete;
    World &operator=(World const &that) = delete;

    void get_entities_positions(Vector<EntityPos> &out) const;
    Entity &create_player();
    void tick();
private:
    template<typename... Args>
    Entity &create_entity(Args &&...args);

    std::list<Entity> entity_storage;
    data::Config const &config;
    data::Database const &db;

    PhysicsEngine physics_engine;
};

template<typename... Args>
Entity &World::create_entity(Args &&...args)
{
    entity_storage.emplace_back(*this, args...);
    Entity &result = entity_storage.back();
    auto pos = result.get_pos();
    LUX_LOG("WORLD", DEBUG,
            "created new entity, typeid: %s, pos: %.2f, %.2f, %.2f",
            db.get_entity_id(result.type.str_id), pos.x, pos.y, pos.z);
    return result;
}
