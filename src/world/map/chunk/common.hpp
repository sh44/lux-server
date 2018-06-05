#pragma once

#include <alias/int.hpp>
#include <linear/point_3d.hpp>

namespace world
{

typedef I32 ChunkCoord;
typedef linear::Point3d<ChunkCoord> ChunkPoint;
typedef SizeT ChunkIndex;

}
