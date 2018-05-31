#pragma once

#include <linear/size_2d.hpp>

namespace net
{

struct ClientData
{
    linear::Size2d<uint16_t> view_size; //in tiles
};

}
