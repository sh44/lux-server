#pragma once

#include <lux/alias/scalar.hpp>
#include <lux/alias/pos_map.hpp>
#include <lux/alias/vec_2.hpp>
#include <lux/common.hpp>
#include <lux/world/map.hpp>

namespace data { struct Config; }

struct Chunk;

class Generator
{
public:
    Generator(data::Config const &config);
    LUX_NO_COPY(Generator);

    void generate_chunk(Chunk &chunk, ChkPos const &pos);
private:
    PosMap<Vec2<ChkCoord>, F32[CHK_VOLUME]> height_map;

    data::Config const &config;
};
