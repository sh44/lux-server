#pragma once

#include <btBulletDynamicsCommon.h>
//
#include <lux/alias/scalar.hpp>
#include <lux/alias/vector.hpp>
#include <lux/alias/vec_3.hpp>
//
#include <tile/tile.hpp>

struct Chunk
{
    ~Chunk()
    {
        if(mesh      != nullptr) delete mesh;
        if(triangles != nullptr) delete triangles;
    }
    Vector<Tile> tiles;
    //TODO not perfect, since size is known at compile time,
    // but still better than an array that cannot be initialized using custom
    // constructor
    Vector<Vec3<F32>>   vertices;
    Vector<I32>                 indices;
    btBvhTriangleMeshShape     *mesh = nullptr;
    btTriangleIndexVertexArray *triangles = nullptr;
};
