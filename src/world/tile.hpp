#pragma once

#include <alias/ref.hpp>
#include <world/tile/type.hpp>

namespace world
{

class Tile
{
    public:
    Tile(const tile::Type &type = tile::default_type);

    Ref<const tile::Type> type;
};

}
