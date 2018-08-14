#pragma once

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

    data::Config const &config;
};

}
