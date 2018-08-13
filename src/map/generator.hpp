#pragma once

#include <lux/common/map.hpp>
//
#include <physics_engine.hpp>

namespace data { struct Config; }

namespace map
{

struct Chunk;

class Generator
{
public:
    Generator(PhysicsEngine &physics_engine, data::Config const &config);
    Generator(Generator const &that) = delete;
    Generator &operator=(Generator const &that) = delete;

    void generate_chunk(Chunk &chunk, ChkPos const &pos);
private:
    void create_mesh(Chunk &chunk, ChkPos const &pos);

    data::Config const &config;
    PhysicsEngine &physics_engine;
};

}
