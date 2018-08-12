#pragma once

#include <btBulletDynamicsCommon.h>
//
#include <lux/alias/array.hpp>
#include <lux/alias/scalar.hpp>
#include <lux/alias/vector.hpp>
#include <lux/common/map.hpp>

namespace map
{

struct TileType;

struct Chunk
{
    ~Chunk()
    {
        if(mesh      != nullptr) delete mesh;
        if(triangles != nullptr) delete triangles;
    }
    Array<TileType const *, CHK_VOLUME> tiles;

    Vector<Vec3<F32>> vertices;
    Vector<I32>       indices;

    btBvhTriangleMeshShape     *mesh      = nullptr;
    btTriangleIndexVertexArray *triangles = nullptr;
};

}
