#pragma once

#include <functional>
//
#include <glm/detail/type_vec3.hpp>

namespace linear
{

template<typename T>
using Point3d = glm::tvec3<T>;

}

namespace std
{

using namespace linear;

template<typename T>
struct hash<Point3d<T>>
{
    size_t operator()(Point3d<T> const &k) const
    {
        return (((hash<T>()(k.x) ^
                 (hash<T>()(k.y) << 1))>> 1) ^
                 (hash<T>()(k.z) << 1))>> 1;
    }
};

}
