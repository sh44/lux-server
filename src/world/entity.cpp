#include "entity.hpp"

namespace world::entity
{

Entity::Entity(Type const &type, Point pos) :
    type(type),
    pos(pos)
{

}

}
