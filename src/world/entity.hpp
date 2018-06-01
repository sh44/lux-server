#pragma once

#include <world/entity/point.hpp>
#include <world/entity/type.hpp>

namespace world
{ 

class World;

inline namespace entity
{

class Entity
{
public:
    Entity(World const &world, Type const &type, Point pos);

    Point get_pos() const;

    World const &world;
    Type const &type;
private:
    Point pos;
};

}
}
