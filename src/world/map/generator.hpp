#pragma once

#include <world/map/chunk/common.hpp>

namespace data { class Config; }

namespace world
{

class Chunk;

class Generator
{
    public:
    Generator(data::Config const &config);

    void generate_chunk(Chunk &chunk, ChunkPoint pos);
    private:
    data::Config const &config;
};

}
