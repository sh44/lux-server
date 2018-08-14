#pragma once

#include <lux/alias/scalar.hpp>
#include <lux/alias/string.hpp>
#include <lux/common/tile.hpp>

namespace map
{

struct TileType
{
    enum Shape : U8
    {
        EMPTY,
        FLOOR,
        WALL
    };

    TileType(String const &_str_id, String const &_name, Shape _shape) :
        str_id(_str_id), name(_name), shape(_shape)
    {

    }

    tile::Id   id;
    String str_id;
    String name;
    Shape shape;
};

}
