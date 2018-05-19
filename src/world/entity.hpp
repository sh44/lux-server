#pragma once

#include <world/entity/point.hpp>
#include <world/entity/type.hpp>

namespace world
{ 
inline namespace entity
{

class Entity
{
public:
    Entity(Type const &type, Point pos);

    Type const &type;
private:
    Point pos;
};

}
}
