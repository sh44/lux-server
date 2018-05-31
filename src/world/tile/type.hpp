#pragma once

#include <cstdint>
//
#include <linear/point_2d.hpp>

namespace world::tile
{

struct Type
{
    enum : uint8_t
    {
        EMPTY,
        FLOOR,
        WALL
    } shape;

    linear::Point2d<uint8_t> tex_pos; //TODO typedef?
};

}
