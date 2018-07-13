#pragma once

#include <alias/int.hpp>
#include <linear/point_2d.hpp>

struct TileType
{
    typedef linear::Point2d<U8> TexPos;
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
