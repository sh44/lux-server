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

    Chunk() : mesh(nullptr) { }
    ~Chunk()
    {
        if(mesh      != nullptr) delete mesh;
    }
};

}
