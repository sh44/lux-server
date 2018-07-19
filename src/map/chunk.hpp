#pragma once

#include <lux/alias/array.hpp>
#include <lux/common/chunk.hpp>
//
#include <tile/tile.hpp>

struct Chunk
{
    Array<Tile, chunk::TILE_SIZE> tiles;
};
