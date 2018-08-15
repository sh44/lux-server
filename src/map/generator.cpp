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
    Vec2<ChkCoord> h_pos = Vec2<ChkCoord>(pos);
    if(height_map.count(h_pos) == 0)
    {
        height_map[h_pos];
        for(SizeT i = 0; i < CHK_VOLUME; ++i)
        {
            MapPos map_pos = to_map_pos(pos, i);
            Vec2<F32> c_pos = (Vec2<F32>)map_pos + Vec2<F32>(0.5f, 0.5f);
            F32 o1 = glm::simplex(c_pos * 0.01f);
            F32 o2 = glm::simplex(c_pos * 0.04f);
            F32 o3 = glm::simplex(c_pos * 0.08f);
            height_map[pos][i] =
                std::pow((o1 + o2 * 0.5f + o3 * 0.25f) / 1.75f, 3.f)
                * 40.f - 25.f;
        }
    }
    for(SizeT i = 0; i < CHK_VOLUME; ++i)
    {
        MapPos map_pos = to_map_pos(pos, i);
        auto const *tile_type = &config.db->get_tile("void");
        F32 h = height_map[pos][i];
        if(map_pos.z <= h)
        {
            if(map_pos.z < h - 5.f)
            {
                tile_type = &config.db->get_tile("raw_stone");
            }
            else
            {
                if(std::hash<F32>()(h) % 32 == 0)
                {
                    tile_type = &config.db->get_tile("dirt");
                }
                else if(std::hash<F32>()(h) % 8 == 0)
                {
                    tile_type = &config.db->get_tile("gravel");
                }
                else
                {
                    tile_type = &config.db->get_tile("raw_stone");
                }
            }
        }
        else
        {
            tile_type = &config.db->get_tile("void");
        }
        chunk.tiles[i] = tile_type;
    }
}

}
