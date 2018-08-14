#include <type_traits>
#include <functional>
//
#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>
//
#include <lux/alias/scalar.hpp>
#include <lux/alias/vec_2.hpp>
#include <lux/alias/vec_3.hpp>
//
#include <data/database.hpp>
#include <data/config.hpp>
#include <map/chunk.hpp>
#include <map/tile_type.hpp>
#include "generator.hpp"

namespace map
{

Generator::Generator(PhysicsEngine &physics_engine, data::Config const &config) :
    config(config),
    physics_engine(physics_engine)
{

}

void Generator::generate_chunk(Chunk &chunk, ChkPos const &pos)
{
    for(SizeT i = 0; i < CHK_VOLUME; ++i)
    {
        Vec3<U8> r_size = {8, 8, 4};
        auto u_mod = [&](MapCoord c, U8 s) -> U8
        {
            //TODO make more general functions from this and chunk:: functions
            return (*(std::make_unsigned<MapCoord>::type *)(&c)) % s;
        };
        MapPos map_pos = to_map_pos(pos, i);
        auto hash = std::hash<MapPos>()(map_pos);
        auto const *tile_type = config.db->tile_types.at("void");
        if(u_mod(map_pos.z, r_size.z) == 0)
        {
            if(u_mod(map_pos.x, r_size.x) < 3 ||
               u_mod(map_pos.x, r_size.x) > 5 ||
               u_mod(map_pos.y, r_size.y) < 3 ||
               u_mod(map_pos.y, r_size.y) > 5)
            {
                tile_type = config.db->tile_types.at("stone_floor");
            }
        }
        else if(u_mod(map_pos.x, r_size.x) == 0 ||
                u_mod(map_pos.y, r_size.y) == 0)
        {
            if((u_mod(map_pos.z, r_size.z) == 1 && hash % 6 == 0) ||
               (u_mod(map_pos.z, r_size.z) == 2 &&
                std::hash<MapPos>()(map_pos - MapPos(0, 0, 1)) % 6 == 0))
            {
                tile_type = config.db->tile_types.at("void");
            }
            else
            {
                tile_type = config.db->tile_types.at("stone_wall");
            }
        }
        Vec2<F32> centered_pos = (Vec2<F32>)map_pos + Vec2<F32>(0.5, 0.5);
        Vec2<F32> h1_seed = centered_pos + Vec2<F32>(0, 0);
        Vec2<F32> h2_seed = centered_pos + Vec2<F32>(1203.f, -102.f);
        F32 h1 = (glm::simplex(h1_seed * 0.01f) * 10.f) - 25.f;
        F32 h2 = std::pow(glm::simplex(h2_seed * 0.005f), 2);
        if(map_pos.z > h1 && map_pos.z < h1 + 20.f)
        {
            if(map_pos.z > h1 + 13.f)
            {
                if(map_pos.z > h1 + 19.f)
                {
                    tile_type = config.db->tile_types.at("grass");
                }
                else
                {
                    tile_type = config.db->tile_types.at("dirt");
                }
            }
            else
            {
                tile_type = config.db->tile_types.at("raw_stone");
            }
        }
        else if(map_pos.z >= h1 + 20.f)
        {
            tile_type = config.db->tile_types.at("void");
        }
        if(h2 > 0.92f && map_pos.z > h1 - 20.f)
        {
            tile_type = config.db->tile_types.at("void");
        }
        chunk.tiles[i] = tile_type;
    }
    create_mesh(chunk, pos);
}

void Generator::create_mesh(Chunk &chunk, ChkPos const &pos)
{
    //TODO merge this with client version
    {
        SizeT worst_case_len = CHK_VOLUME / 2 +
            (CHK_VOLUME % 2 == 0 ? 0 : 1);
        /* this is the size of a checkerboard pattern, the worst case for this
         * algorithm.
         */
        chunk.vertices.reserve(worst_case_len * 6 * 4); //TODO magic numbers
        chunk.indices.reserve(worst_case_len * 6 * 6);  //
    }
    
    I32 index_offset = 0;
    constexpr glm::vec3 quads[6][4] =
    {
        {{0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 1.0, 1.0}, {0.0, 0.0, 1.0}},
        {{1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {1.0, 1.0, 1.0}, {1.0, 0.0, 1.0}},
        {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {1.0, 0.0, 1.0}, {0.0, 0.0, 1.0}},
        {{0.0, 1.0, 0.0}, {1.0, 1.0, 0.0}, {1.0, 1.0, 1.0}, {0.0, 1.0, 1.0}},
        {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 1.0, 0.0}},
        {{0.0, 0.0, 1.0}, {1.0, 0.0, 1.0}, {1.0, 1.0, 1.0}, {0.0, 1.0, 1.0}}
    };
    constexpr MapPos offsets[6] =
        {{-1,  0,  0}, { 1,  0,  0},
         { 0, -1,  0}, { 0,  1,  0},
         { 0,  0, -1}, { 0,  0,  1}};

    auto is_solid = [&](IdxPos const &idx_pos) -> bool
    {
        return to_chk_pos(idx_pos) != ChkPos(0, 0, 0) ||
               chunk.tiles[to_chk_idx(idx_pos)]->id != "void";
    };

    for(SizeT i = 0; i < CHK_VOLUME; ++i)
    {
        IdxPos idx_pos = to_idx_pos(i);
        if(chunk.tiles[to_chk_idx(idx_pos)]->id != "void")
        {
            for(SizeT side = 0; side < 6; ++side)
            {
                if(!is_solid((MapPos)idx_pos + offsets[side]))
                {
                    for(unsigned j = 0; j < 4; ++j)
                    {
                        chunk.vertices.emplace_back((Vec3<F32>)idx_pos +
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
    chunk.vertices.shrink_to_fit();
    chunk.indices.shrink_to_fit();

    if(chunk.vertices.size() != 0)
    {
        chunk.triangles = new btTriangleIndexVertexArray(
            chunk.indices.size() / 3,
            chunk.indices.data(),
            sizeof(U32) * 3,
            chunk.vertices.size(),
            (F32 *)chunk.vertices.data(),
            sizeof(Vec3<F32>));
        chunk.mesh = new btBvhTriangleMeshShape(chunk.triangles, false,
                {0.0, 0.0, 0.0}, {CHK_SIZE.x, CHK_SIZE.y, CHK_SIZE.z});
        physics_engine.add_shape(to_map_pos(pos, 0), chunk.mesh);
    }

}

}
