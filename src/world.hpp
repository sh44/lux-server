#pragma once

#include <list>
//
#include <alias/ref.hpp>
#include <util/log.hpp>
#include <data/config.hpp>
#include <world/entity.hpp>
#include <world/map.hpp>

namespace world
{

class World
{
public:
    World(data::Config const &config);
    World(World const &that) = delete;
    World &operator=(World const &that) = delete;

    Tile       &operator[](MapPoint const &pos);
    Tile const &operator[](MapPoint const &pos) const;

    Entity &create_player();
    void update();
private:
    template<typename... Args>
    Entity &create_entity(Args const &...args);

    std::list<Entity> entity_storage;
    data::Config const &config;

    Map map;
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

}
