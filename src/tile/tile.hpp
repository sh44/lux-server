#pragma once

namespace tile { struct Type; }

struct Tile
{
    Tile(tile::Type const &type) : type(&type) { }
    tile::Type const *type;
};
