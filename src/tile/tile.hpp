#pragma once

#include <tile/type.hpp>

class Tile
{
    public:
    Tile() = default;
    Tile(tile::Type const &type);

    tile::Type const *type;
};
