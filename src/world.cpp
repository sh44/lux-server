#include "world.hpp"

namespace world
{

Entity &World::create_player()
{
    return create_entity();
}

}
