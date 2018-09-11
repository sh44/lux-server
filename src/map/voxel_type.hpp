#pragma once

#include <lux/alias/scalar.hpp>
#include <lux/alias/string.hpp>
#include <lux/common.hpp>

struct VoxelType
{
    LUX_NO_COPY(VoxelType);
    LUX_MOVEABLE(VoxelType);

    enum Shape : U8
    {
        EMPTY,
        FLOOR,
        WALL
    };

    VoxelType(String const &_str_id, String const &_name, Shape _shape) :
        str_id(_str_id), name(_name), shape(_shape)
    {

    }

    String str_id;
    String name;
    Shape shape;
};
