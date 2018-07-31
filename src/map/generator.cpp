#include <iostream>
#include <functional>
//
#include <lux/alias/scalar.hpp>
//
#include <data/database.hpp>
#include <data/config.hpp>
#include <map/chunk.hpp>
#include <tile/type.hpp>
#include "generator.hpp"

Generator::Generator(PhysicsEngine &physics_engine, data::Config const &config) :
    config(config),
    physics_engine(physics_engine)
{

}

void Generator::generate_chunk(Chunk &chunk, chunk::Pos const &pos)
{
    num_gen.seed(std::hash<chunk::Pos>()(pos));
    chunk.tiles.reserve(chunk::TILE_SIZE);
    for(SizeT i = 0; i < chunk::TILE_SIZE; ++i)
    {
        map::Pos map_pos = chunk::to_map_pos(pos, i);
        if(map_pos.x % 8 == 0 ||
           map_pos.y % 8 == 0 ||
           map_pos.z % 4 == 0)
        {
            if(num_gen() % 6 == 5)
            {
                chunk.tiles.emplace_back(*config.db->tile_types.at("void"));
            }
            else
            {
                chunk.tiles.emplace_back(*config.db->tile_types.at("stone_wall"));
            }
        }
        else
        {
            chunk.tiles.emplace_back(*config.db->tile_types.at("void"));
        }
    }

    /* MESHING BEGINS */
    //TODO merge this with client version
    
    auto is_solid = [&](map::Pos const &map_pos) -> bool
    {
        if(chunk::to_pos(map_pos) != chunk::Pos(0, 0, 0)) return false;
        return chunk.tiles[chunk::to_index(map_pos)].type->id != "void";
    };

    I32 index_offset = 0;
    const glm::vec3 quads[6][4] =
    {
        {{0.0, 0.0, 0.0},
         {0.0, 1.0, 0.0},
         {0.0, 1.0, 1.0},
         {0.0, 0.0, 1.0}},
        {{1.0, 0.0, 0.0},
         {1.0, 1.0, 0.0},
         {1.0, 1.0, 1.0},
         {1.0, 0.0, 1.0}},
        {{0.0, 0.0, 0.0},
         {1.0, 0.0, 0.0},
         {1.0, 0.0, 1.0},
         {0.0, 0.0, 1.0}},
        {{0.0, 1.0, 0.0},
         {1.0, 1.0, 0.0},
         {1.0, 1.0, 1.0},
         {0.0, 1.0, 1.0}},
        {{0.0, 0.0, 0.0},
         {1.0, 0.0, 0.0},
         {1.0, 1.0, 0.0},
         {0.0, 1.0, 0.0}},
        {{0.0, 0.0, 1.0},
         {1.0, 0.0, 1.0},
         {1.0, 1.0, 1.0},
         {0.0, 1.0, 1.0}}
    };

    for(SizeT i = 0; i < chunk::TILE_SIZE; ++i)
    {
        map::Pos map_pos = chunk::to_map_pos({0, 0, 0}, i);
        if(is_solid(map_pos))
        {
            const map::Pos offsets[6] = {{-1,  0,  0},
                                         { 1,  0,  0},
                                         { 0, -1,  0},
                                         { 0,  1,  0},
                                         { 0,  0, -1},
                                         { 0,  0,  1}};
            for(SizeT side = 0; side < 6; ++side)
            {
                if(!is_solid(map_pos + offsets[side]))
                {
                    for(unsigned j = 0; j < 4; ++j)
                    {
                        chunk.vertices.emplace_back((linear::Vec3<F32>)map_pos +
                                                    quads[side][j]);
                    }
                    for(auto const &idx : {0, 1, 2, 2, 3, 0})
                    {
                        chunk.indices.emplace_back(idx + index_offset);
                    }
                    index_offset += 4;
                }
            }
        }
    }

    if(chunk.vertices.size() != 0)
    {
        chunk.triangles = new btTriangleIndexVertexArray(chunk.indices.size() / 3,
                                                         chunk.indices.data(),
                                                         sizeof(U32) * 3,
                                                         chunk.vertices.size(),
                                                         (F32 *)chunk.vertices.data(),
                                                         sizeof(linear::Vec3<F32>));
        chunk.mesh = new btBvhTriangleMeshShape(chunk.triangles, true);
        physics_engine.add_shape(chunk::to_map_pos(pos, 0), chunk.mesh);
    }

    /* MESHING ENDS */
}
