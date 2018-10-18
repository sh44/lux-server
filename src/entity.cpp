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
    //@TODO
    entities[id] = id;
    comps.pos[id]       = pos;
    comps.vel[id]       = {0, 0, 0};
    comps.shape[id].rad = 0.8;
    return id;
}

EntityHandle create_player() {
    LUX_LOG("creating new player");
    return create_entity({1, 1, 0});
}

static bool check_collision(EntityVec const& pos, F32 rad) {
    MapPos map_pos = (MapPos)pos;
    Int bound = std::ceil(rad);
    for(MapCoord y = map_pos.y - bound; y <= map_pos.y + bound; ++y) {
        for(MapCoord x = map_pos.x - bound; x <= map_pos.x + bound; ++x) {
            guarantee_chunk(to_chk_pos({x, y, pos.z}));
            VoxelType const& vox = get_voxel_type({x, y, pos.z});
            F32 sq_dst = 0.0f;
            Vec2F min = {x, y};
            Vec2F max = min + Vec2F(1.f, 1.f);
            for(Uns i = 0; i < 2; i++) {
                float v = Vec2F(pos)[i];
                if(v < min[i]) sq_dst += (min[i] - v) * (min[i] - v);
                if(v > max[i]) sq_dst += (v - max[i]) * (v - max[i]);
            }
            if(vox.shape == VoxelType::BLOCK && sq_dst <= rad) {
                return true;
            }
        }
    }
    return false;
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
                    auto& rad = comps.shape.at(id).rad;
                    VoxelType const& standing_voxel = get_voxel_type(pos);
                    if(standing_voxel.shape == VoxelType::EMPTY) {
                        vel.z = -1.f;
                    }
                    //@TODO fix negative axis
                    if(!check_collision({new_pos.x, pos.y, pos.z}, rad)) {
                        pos.x = new_pos.x;
                    }
                    if(!check_collision({pos.x, new_pos.y, pos.z}, rad)) {
                        pos.y = new_pos.y;
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
