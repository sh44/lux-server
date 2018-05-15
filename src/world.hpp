#pragma once

#include <list>
#include <unordered_map>
//
#include <world/entity.hpp>
#include <world/map.hpp>

class World
{
public:
    world::Entity &create_player();
private:
    template<typename... Args>
    world::Entity &create_entity(Args... args);

    world::Map map;

    std::list<world::Entity> entity_storage;
};

template<typename... Args>
world::Entity &World::create_entity(Args... args)
{
    entity_storage.emplace_back(args...);
    return entity_storage.back();
}
