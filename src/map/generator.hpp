#pragma once

#include <lux/alias/scalar.hpp>
#include <lux/alias/hash_map.hpp>
#include <lux/alias/vec_2.hpp>
#include <lux/common/map.hpp>

namespace data { struct Config; }

namespace map
{

struct Chunk;

class Generator
{
public:
    Generator(data::Config const &config);
    Generator(Generator const &that) = delete;
    Generator &operator=(Generator const &that) = delete;

    void generate_chunk(Chunk &chunk, ChkPos const &pos);
private:
    HashMap<Vec2<ChkCoord>, F32[CHK_VOLUME]> height_map;

    data::Config const &config;
};

}
