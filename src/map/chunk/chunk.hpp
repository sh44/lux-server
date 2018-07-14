#pragma once

#include <alias/int.hpp>
#include <linear/vec_3.hpp>
#include <map/chunk/common.hpp>
#include <map/common.hpp>
#include <tile/tile.hpp>

struct Chunk
{
    static constexpr linear::Vec3<U16> SIZE = {16, 16, 3};
    static const SizeT TILE_SIZE = SIZE.x * SIZE.y * SIZE.z;

    Chunk(Tile *tiles);
    Chunk(Chunk const &that) = delete;
    Chunk operator=(Chunk const &that) = delete;

    static ChunkPoint point_map_to_chunk(MapPoint const &point);
    static ChunkIndex point_map_to_index(MapPoint const &point);

    Tile *tiles;
};
