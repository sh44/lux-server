#pragma once

#include <lux/alias/scalar.hpp>
#include <lux/alias/c_string.hpp>
#include <lux/linear/vec_2.hpp>

namespace tile
{

struct Type
{
    typedef linear::Vec2<U8> TexPos;
    enum Shape : U8
    {
        EMPTY,
        FLOOR,
        WALL
    };

    CString id;
    CString name;
    Shape shape;
    TexPos tex_pos;
};

}
