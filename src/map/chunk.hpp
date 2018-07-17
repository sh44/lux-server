#pragma once

#include <lux/alias/scalar.hpp>
#include <lux/linear/vec_3.hpp>
#include <lux/common/chunk.hpp>
#include <lux/common/map.hpp>
//
#include <tile/tile.hpp>

struct Chunk
{
    static const ChunkSize &SIZE;
    static const SizeT     &TILE_SIZE;

    Chunk(Tile *tiles);
    Chunk(Chunk const &that) = delete;
    Chunk operator=(Chunk const &that) = delete;

    Tile *tiles;
};
