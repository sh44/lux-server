#include <lux/common/entity.hpp>
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
            iter->update();
            iter++;
        }
    }
    physics_engine.update();
}

Entity &World::create_player()
{
    util::log("WORLD", util::DEBUG, "created player");
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
