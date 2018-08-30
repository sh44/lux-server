#pragma once

#include <lux/alias/array.hpp>
#include <lux/common/map.hpp>
//
#include <map/mesh.hpp>

namespace map
{

struct TileType;

struct Chunk
{
    Array<TileType const *, CHK_VOLUME> tiles;

    /* mesh should be deleted when the chunk is unloaded (if it exists)
     * not that even if is_mesh_generated == true, the mesh might not exist,
     * since completely solid, or completely empty chunks have no meshes */
    Mesh *mesh = nullptr;
    bool is_mesh_generated;
};

}
