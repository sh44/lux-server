#pragma once

#include <lux_shared/common.hpp>

struct McVert {
    Vec3F p;
    U8  idx;
};

constexpr Arr<Vec3F, 8> marching_cubes_offsets = {
    {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
    {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}
};

//@CONSIDER using a single U64 for input grid
Uns marching_cubes(Arr<McVert, 5 * 3>* out, Arr<F32, 8> const& grid);
