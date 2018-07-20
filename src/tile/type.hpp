#pragma once

#include <lux/alias/scalar.hpp>
#include <lux/alias/c_string.hpp>

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

    CString id;
    CString name;
    Shape shape;
};

}
