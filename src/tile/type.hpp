#pragma once

#include <lux/alias/scalar.hpp>
#include <lux/alias/string.hpp>

namespace tile
{

struct Type
{
    enum Shape : U8
    {
        EMPTY,
        FLOOR,
        WALL
    };

    String id;
    String name;
    Shape shape;
};

}
