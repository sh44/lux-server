#pragma once

#include <lux/util/num_gen.hpp>
#include <lux/common/chunk.hpp>

namespace data { struct Config; }

struct Chunk;

class Generator
{
    public:
    Generator(data::Config const &config);
    Generator &operator=(Generator const &that) = delete;

    void generate_chunk(Chunk &chunk, ChunkPos const &pos);
    private:
    data::Config const &config;
    util::NumGen num_gen;
};
