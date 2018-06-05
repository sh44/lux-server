#pragma once

#include <alias/int.hpp>
#include <linear/size_3d.hpp>
#include <world/map/chunk/common.hpp>
#include <world/map/common.hpp>
#include <world/tile.hpp>

namespace world
{

struct Chunk
{
    static constexpr linear::Size3d<U16> SIZE = {16, 16, 3};
    static const SizeT TILE_SIZE = SIZE.x * SIZE.y * SIZE.z;

    Chunk(Tile *tiles);
    Chunk(Chunk const &that) = delete;
    Chunk operator=(Chunk const &that) = delete;

    static ChunkPoint point_map_to_chunk(MapPoint point);
    static ChunkIndex point_map_to_index(MapPoint point);

    Tile *tiles;
};

}
