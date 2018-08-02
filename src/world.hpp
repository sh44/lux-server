#pragma once

#include <list>
//
#include <lux/alias/ref.hpp>
#include <lux/alias/vector.hpp>
#include <lux/util/log.hpp>
//
#include <data/config.hpp>
#include <entity/entity.hpp>
#include <entity/type.hpp>
#include <map/map.hpp>

class World : public Map
{
public:
    World(data::Config const &config);
    World(World const &that) = delete;
    World &operator=(World const &that) = delete;

    using Map::operator[];

    void get_entities_positions(Vector<entity::Pos> &out) const;
    Entity &create_player();
    void update();
private:
    template<typename... Args>
    Entity &create_entity(Args const &...args);

    std::list<Entity> entity_storage;
    data::Config const &config;

    PhysicsEngine physics_engine;
};

template<typename... Args>
Entity &World::create_entity(Args const &...args)
{
    entity_storage.emplace_back(Ref<World>(*this), args...);
    Entity &result = entity_storage.back();
    auto pos = result.get_pos();
    util::log("WORLD", util::DEBUG, "created new entity, typeid: %s, pos: %.2f, %.2f, %.2f",
              result.type.id, pos.x, pos.y, pos.z);
    return result;
}
