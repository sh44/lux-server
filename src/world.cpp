#include <world/entity/point.hpp>
#include <obj.hpp>
#include "world.hpp"

namespace world
{

Entity &World::create_player()
{
    return create_entity(obj::Human, entity::Point(0, 0, 0));
}

}
