#pragma once

#include <world/tile/type.hpp>

namespace world
{

class Tile
{
    public:
    Tile(TileType const &type);
    Tile(Tile const &that) = delete;
    Tile &operator=(Tile const &that) = delete;

    TileType const *type;
};

}
