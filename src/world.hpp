#pragma once

#include <list>
//
#include <data/config.hpp>
#include <world/entity.hpp>
#include <world/map.hpp>

namespace world
{

class World
{
public:
    World(data::Config const &config);

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
    return entity_storage.back();
}

}
