#pragma once

#include <lux/util/num_gen.hpp>
#include <lux/common/map.hpp>
//
#include <physics_engine.hpp>

namespace data { struct Config; }

struct Chunk;

class Generator
{
    public:
    Generator(PhysicsEngine &physics_engine, data::Config const &config);
    Generator &operator=(Generator const &that) = delete;

    void generate_chunk(Chunk &chunk, ChkPos const &pos);
    private:
    data::Config const &config;
    util::NumGen num_gen;
    PhysicsEngine &physics_engine;
};
