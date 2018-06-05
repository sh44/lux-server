#pragma once

#include <world/tile/type.hpp>

namespace world
{

class Tile
{
    public:
    Tile(const TileType &type);

    TileType const *type;
};

}
