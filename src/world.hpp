#pragma once

#include <list>
#include <unordered_map>
//
#include <world/entity.hpp>
#include <world/map.hpp>

inline namespace world
{

class World
{
public:
    Entity &create_player();
private:
    template<typename... Args>
    Entity &create_entity(Args... args);

    Map map;

    std::list<Entity> entity_storage;
};

template<typename... Args>
Entity &World::create_entity(Args... args)
{
    entity_storage.emplace_back(args...);
    return entity_storage.back();
}

}
