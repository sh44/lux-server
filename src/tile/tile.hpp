#pragma once

#include <tile/tile_type.hpp>

class Tile
{
    public:
    Tile(TileType const &type);
    Tile(Tile const &that) = delete;
    Tile &operator=(Tile const &that) = delete;

    TileType const *type;
};
