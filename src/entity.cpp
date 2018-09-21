#include <lux_shared/containers.hpp>
//
#include <db.hpp>
#include <map.hpp>
#include <entity.hpp>

List<Entity> entities;

Entity& create_entity(EntityVec const& pos) {
    LUX_LOG("creating new entity");
    LUX_LOG("    pos: {%.2f, %.2f, %.2f}", pos.x, pos.y, pos.z);
    Entity& entity = entities.emplace_front();
    entity.pos = pos;
    return entity;
}

Entity& create_player() {
    LUX_LOG("creating new player");
    return create_entity({3, 3, 0});
}

void entities_tick() {
    for(auto& entity : entities) {
        ChkPos chk_pos = to_chk_pos(entity.pos);
        guarantee_chunk(chk_pos + ChkPos( 0,  0,  0));
        guarantee_chunk(chk_pos + ChkPos(-1,  0,  0));
        guarantee_chunk(chk_pos + ChkPos( 1,  0,  0));
        guarantee_chunk(chk_pos + ChkPos( 0, -1,  0));
        guarantee_chunk(chk_pos + ChkPos( 0,  1,  0));
        guarantee_chunk(chk_pos + ChkPos( 0,  0, -1));
        guarantee_chunk(chk_pos + ChkPos( 0,  0,  1));
        VoxelType const& standing_voxel = get_voxel_type(entity.pos);
        if(standing_voxel.shape == VoxelType::EMPTY) {
            entity.vel.z = -1.f;
        }
        EntityVec new_pos = entity.pos + entity.vel;
        {   VoxelType const& new_voxel =
                get_voxel_type({new_pos.x, entity.pos.y, entity.pos.z});
            if(new_voxel.shape != VoxelType::BLOCK) {
                entity.pos.x = new_pos.x;
            }
        }
        {   VoxelType const& new_voxel =
                get_voxel_type({entity.pos.x, new_pos.y, entity.pos.z});
            if(new_voxel.shape != VoxelType::BLOCK) {
                entity.pos.y = new_pos.y;
            }
        }
        entity.vel += -entity.vel * 0.10f;
        add_light_node(entity.pos, {0xF, 0xF, 0xF});
    }
}

void remove_entity(Entity& entity) {
    LUX_LOG("unimplemented");
}
