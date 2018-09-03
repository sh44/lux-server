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
#include <map/voxel_type.hpp>
#include "generator.hpp"

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
        VoxelId voxel_id = config.db->get_voxel_id("void");
        F32 h = height_map[pos][i];
        auto hash = std::hash<MapPos>()(map_pos);
        if(map_pos.z <= h && map_pos.z > h - 20.f)
        {
            if(map_pos.z < h - 5.f)
            {
                voxel_id = config.db->get_voxel_id("raw_stone");
            }
            else
            {
                if(std::hash<F32>()(h) % 32 == 0)
                {
                    voxel_id = config.db->get_voxel_id("dirt");
                }
                else if(std::hash<F32>()(h) % 8 == 0)
                {
                    voxel_id = config.db->get_voxel_id("gravel");
                }
                else
                {
                    voxel_id = config.db->get_voxel_id("raw_stone");
                }
            }
        }
        else if(map_pos.z <= h - 20.f)
        {
            if(map_pos.z % 4 == 0)
            {
                voxel_id = config.db->get_voxel_id("stone_floor");
            }
            else if(map_pos.x % 8 == 0 ||
                    map_pos.y % 8 == 0)
            {
                if((map_pos.z % 4 == -1 && hash % 6 == 0) ||
                   (map_pos.z % 4 == -2 &&
                    std::hash<MapPos>()(map_pos - MapPos(0, 0, 1)) % 6 == 0))
                {
                    voxel_id = config.db->get_voxel_id("void");
                }
                else
                {
                    voxel_id = config.db->get_voxel_id("stone_wall");
                }
            }
            else
            {
                voxel_id = config.db->get_voxel_id("void");
            }
        }
        chunk.voxels[i] = voxel_id;
    }
}
