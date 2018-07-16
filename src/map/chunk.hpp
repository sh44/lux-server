#pragma once

#include <alias/scalar.hpp>
#include <linear/vec_3.hpp>
#include <common/chunk.hpp>
#include <common/map.hpp>
#include <tile/tile.hpp>

struct Chunk
{
    static const ChunkSize &SIZE;
    static const SizeT     &TILE_SIZE;

    Chunk(Tile *tiles);
    Chunk(Chunk const &that) = delete;
    Chunk operator=(Chunk const &that) = delete;

    static ChunkPos   pos_map_to_chunk(MapPos const &pos);
    static ChunkIndex pos_map_to_index(MapPos const &pos);

    Tile *tiles;
};
