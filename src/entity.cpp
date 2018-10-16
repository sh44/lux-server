#include <lux_shared/containers.hpp>
//
#include <db.hpp>
#include <map.hpp>
#include <entity.hpp>

EntityComps comps;
EntityComps& entity_comps = comps;
static SparseDynArr<EntityHandle> entities;

EntityHandle create_entity(EntityVec const& pos) {
    LUX_LOG("creating new entity");
    LUX_LOG("    pos: {%.2f, %.2f, %.2f}", pos.x, pos.y, pos.z);
    EntityHandle id     = entities.emplace();
    comps.pos[id]       = pos;
    comps.vel[id]       = {0, 0, 0};
    comps.shape[id].rad = 1.0;
    return id;
}

EntityHandle create_player() {
    LUX_LOG("creating new player");
    return create_entity({1, 1, 0});
}

void entities_tick() {
    for(auto const& id : entities) {
        if(comps.pos.count(id) > 0) {
            auto& pos = comps.pos.at(id);
            ChkPos chk_pos = to_chk_pos(pos);
            constexpr ChkPos offsets[] = {{0, 0, 0}, {-1, 0, 0}, {1, 0, 0},
                {0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}};
            for(auto const& offset : offsets) {
                guarantee_chunk(chk_pos + offset);
            }
            if(comps.vel.count(id) > 0) {
                auto& vel = comps.vel.at(id);
                EntityVec old_pos = pos;
                EntityVec new_pos = pos + vel;
                if(comps.shape.count(id) > 0) {
                    //@TODO
                    auto& rad = comps.shape.at(id).rad;
                    VoxelType const& standing_voxel = get_voxel_type(pos);
                    if(standing_voxel.shape == VoxelType::EMPTY) {
                        vel.z = -1.f;
                    }
                    {   VoxelType const& new_voxel =
                            get_voxel_type({new_pos.x, pos.y, pos.z});
                        if(new_voxel.shape != VoxelType::BLOCK) {
                            pos.x = new_pos.x;
                        }
                    }
                    {   VoxelType const& new_voxel =
                            get_voxel_type({pos.x, new_pos.y, pos.z});
                        if(new_voxel.shape != VoxelType::BLOCK) {
                            pos.y = new_pos.y;
                        }
                    }
                } else {
                    pos += vel;
                }
                vel *= 0.9f;
                if((MapPos)pos != (MapPos)old_pos) {
                    //del_light_source(old_pos);
                    add_light_source(pos, {0xF, 0xF, 0xF});
                }
            }
        }
    }
}

void remove_entity(EntityHandle entity) {
    LUX_ASSERT(entities.contains(entity));
    entities.erase(entity);
}
