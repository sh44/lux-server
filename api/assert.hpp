#pragma once

#include <linear/point_2d.hpp>
#include <linear/point_3d.hpp>
#include <world/tile/type.hpp>
#include <world/entity/type.hpp>

#define ASSERT_STANDARD_LAYOUT(t) \
    static_assert(std::is_standard_layout<t>::value, \
                  #t " must be standard layout to be compatible with lua API")

namespace linear
{
    ASSERT_STANDARD_LAYOUT(Point2d<int>);
    ASSERT_STANDARD_LAYOUT(Point2d<unsigned>);
    ASSERT_STANDARD_LAYOUT(Point2d<float>);
    ASSERT_STANDARD_LAYOUT(Point2d<double>);
    ASSERT_STANDARD_LAYOUT(Point3d<int>);
    ASSERT_STANDARD_LAYOUT(Point3d<unsigned>);
    ASSERT_STANDARD_LAYOUT(Point3d<float>);
    ASSERT_STANDARD_LAYOUT(Point3d<double>);
}

namespace world
{
    ASSERT_STANDARD_LAYOUT(entity::Type);
    ASSERT_STANDARD_LAYOUT(tile::Type);
}
