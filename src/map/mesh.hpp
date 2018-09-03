#pragma once

#include <btBulletDynamicsCommon.h>
//
#include <lux/alias/scalar.hpp>
#include <lux/alias/vec_3.hpp>
#include <lux/alias/vector.hpp>

struct Mesh
{
    Vector<Vec3F> vertices;
    Vector<I32>       indices;

    btBvhTriangleMeshShape     *bt_mesh;
    btTriangleIndexVertexArray *bt_trigs;

    Mesh() : bt_mesh(nullptr), bt_trigs(nullptr) { }
    ~Mesh()
    {
        if(bt_mesh  != nullptr) delete bt_mesh;
        if(bt_trigs != nullptr) delete bt_trigs;
    }
};
