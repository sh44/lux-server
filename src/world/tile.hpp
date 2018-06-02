#pragma once

#include <world/tile/type.hpp>

namespace world
{
inline namespace tile
{

class Tile
{
    public:
    Tile(const Type &type);

    Type const *type;
};

}
}
