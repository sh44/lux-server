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

    Mesh *mesh;
    bool has_mesh;

    Chunk() : mesh(nullptr), has_mesh(false) { }
    ~Chunk()
    {
        if(mesh != nullptr) delete mesh;
    }
};

}
