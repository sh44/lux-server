#pragma once

#include <alias/int.hpp>
#include <linear/point_2d.hpp>

namespace world::tile
{

struct Type
{
    typedef linear::Point2d<U8> TexPos;
    enum Shape : uint8_t
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

}
