#include <lux/common.hpp>
#include <lux/world/entity.hpp>
//
#include <data/database.hpp>
#include <entity/entity_type.hpp>
#include "world.hpp"

World::World(data::Config const &_config) :
    Map(physics_engine, _config),
    config(_config),
    db(*config.db)
{

}

void World::tick()
{
    Map::tick();
    auto iter = entity_storage.begin();
    while(iter != entity_storage.end())
    {
        if(iter->deletion_mark)
        {
            entity_storage.erase(iter);
            iter = entity_storage.begin();
        }
        else
        {
            iter->tick();
            iter++;
        }
    }
    physics_engine.tick();
}

Entity &World::create_player()
{
    LUX_LOG("WORLD", DEBUG, "created player");
    return create_entity(db.entities[config.player_type],
        physics_engine.add_entity({2, 2, -60.5}));
}

void World::get_entities_positions(Vector<EntityPos> &out) const
{
    out.reserve(entity_storage.size());
    for(auto &entity : entity_storage)
    {
        out.emplace_back(entity.get_pos());
    }
}
