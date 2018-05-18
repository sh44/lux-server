#pragma once

#include <alias/ref.hpp>
#include <world/tile/type.hpp>

namespace world
{
inline namespace tile
{

class Tile
{
    public:
    Tile(const Type &type = default_type);

    Ref<const Type> type;
};

}
}
