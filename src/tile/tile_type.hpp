#pragma once

#include <alias/int.hpp>
#include <linear/vec_2.hpp>

struct TileType
{
    typedef linear::Vec2<U8> TexPos;
    enum Shape : U8
    {
        EMPTY,
        FLOOR,
        WALL
    };

    const char *id;
    const char *name;
    Shape shape;
    TexPos tex_pos;
};
