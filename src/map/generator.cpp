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

Generator::Generator(data::Config const &config) :
    config(config)
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
        auto const *tile_type = &config.db->get_tile("void");
        if(u_mod(map_pos.z, r_size.z) == 0)
        {
            tile_type = &config.db->get_tile("stone_floor");
        }
        else if(u_mod(map_pos.x, r_size.x) == 0 ||
                u_mod(map_pos.y, r_size.y) == 0)
        {
            if((u_mod(map_pos.z, r_size.z) == 1 && hash % 6 == 0) ||
               (u_mod(map_pos.z, r_size.z) == 2 &&
                std::hash<MapPos>()(map_pos - MapPos(0, 0, 1)) % 6 == 0))
            {
                tile_type = &config.db->get_tile("void");
            }
            else
            {
                tile_type = &config.db->get_tile("stone_wall");
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
                    tile_type = &config.db->get_tile("grass");
                }
                else
                {
                    tile_type = &config.db->get_tile("dirt");
                }
            }
            else
            {
                tile_type = &config.db->get_tile("raw_stone");
            }
        }
        else if(map_pos.z >= h1 + 20.f)
        {
            tile_type = &config.db->get_tile("void");
        }
        if(h2 > 0.85f && map_pos.z > h1 - 20.f)
        {
            tile_type = &config.db->get_tile("void");
        }
        chunk.tiles[i] = tile_type;
    }
}

}
