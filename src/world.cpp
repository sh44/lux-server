#include <lux/alias/ref.hpp>
#include <lux/common/entity.hpp>
//
#include <entity/type.hpp>
#include "world.hpp"

World::World(data::Config const &config) :
    config(config),
    map(config)
{

}

Tile &World::operator[](map::Pos const &pos)
{
    return map[pos];
}

Tile const &World::operator[](map::Pos const &pos) const
{
    return map[pos];
}

void World::update()
{
    for(auto &entity : entity_storage)
    {
        entity.update();
    }
}

Entity &World::create_player()
{
    util::log("WORLD", util::DEBUG, "created player");
    return create_entity(*config.player_type, entity::Pos(1, 1, 0));
}

void World::get_entities_positions(Vector<entity::Pos> &out) const
{
    out.reserve(entity_storage.size());
    for(auto &entity : entity_storage)
    {
        out.emplace_back(entity.get_pos());
    }
}
