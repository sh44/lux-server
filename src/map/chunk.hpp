#pragma once

#include <lux/alias/array.hpp>
#include <lux/common/chunk.hpp>
//
#include <tile/tile.hpp>

struct Chunk
{
    Vector<Tile> tiles;
    //TODO not perfect, since size is known at compile time,
    // but still better than an array that cannot be initialized using custom
    // constructor
};
