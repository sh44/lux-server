#pragma once

#include <list>
//
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

    Tile       &operator[](MapPoint pos);
    Tile const &operator[](MapPoint pos) const;

    Entity &create_player();
private:
    template<typename... Args>
    Entity &create_entity(Args... args);

    std::list<Entity> entity_storage;
    data::Config const &config;

    Map map;
};

template<typename... Args>
Entity &World::create_entity(Args... args)
{
    entity_storage.emplace_back(args...);
    Entity &result = entity_storage.back();
    auto pos = result.get_pos();
    util::log("WORLD", util::DEBUG, "created new entity, typeid: %s, pos: %f, %f, %f",
              result.type.id, pos.x, pos.y, pos.z);
    return result;
}

}
