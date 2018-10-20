#define GLM_FORCE_PURE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/fast_square_root.hpp>
#include <lux_shared/containers.hpp>
//
#include <db.hpp>
#include <map.hpp>
#include <entity.hpp>

static EntityComps          comps;
SparseDynArr<Entity>        entities;
EntityComps& entity_comps = comps;

EntityHandle create_entity() {
    EntityHandle id = entities.emplace();
    return id;
}

EntityHandle create_player() {
    LUX_LOG("creating new player");
    EntityHandle id       = create_entity();
    comps.pos[id]         = {2, 2, 0};
    comps.vel[id]         = {0, 0, 0};
    comps.sphere[id]      = {0.8f};
    comps.visible[id]     = {0};
    comps.orientation[id] = {0.f};
    return id;
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
    for(auto it = entities.begin(); it != entities.end(); ++it) {
        auto const& id = it.idx;
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
                /*if(comps.shape.count(id) > 0) {
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
                } else {*/
                    pos += vel;
                //}
                vel *= 0.9f;
                if((MapPos)pos != (MapPos)old_pos) {
                    //del_light_source(old_pos);
                    //add_light_source(pos, {0xF, 0xF, 0xF});
                }
            }
        }
    }
}

void remove_entity(EntityHandle entity) {
    LUX_ASSERT(entities.contains(entity));
    entities.erase(entity);
    ///this is only gonna grow longer...
    if(comps.pos.count(entity)          > 0) comps.pos.erase(entity);
    if(comps.vel.count(entity)          > 0) comps.vel.erase(entity);
    if(comps.name.count(entity)         > 0) comps.name.erase(entity);
    if(comps.sphere.count(entity)       > 0) comps.sphere.erase(entity);
    if(comps.rect.count(entity)         > 0) comps.rect.erase(entity);
    if(comps.visible.count(entity)      > 0) comps.visible.erase(entity);
    if(comps.item.count(entity)         > 0) comps.item.erase(entity);
    if(comps.destructible.count(entity) > 0) comps.destructible.erase(entity);
    if(comps.health.count(entity)       > 0) comps.health.erase(entity);
    if(comps.container.count(entity)    > 0) comps.container.erase(entity);
    if(comps.orientation.count(entity)  > 0) comps.orientation.erase(entity);
}

void get_net_entity_comps(NetSsTick::EntityComps* net_comps) {
    //@TODO figure out a way to reduce the verbosity of this function
    for(auto const& pos : comps.pos) {
        net_comps->pos[pos.first] = pos.second;
    }
    for(auto const& name : comps.name) {
        net_comps->name[name.first] = name.second;
    }
    ///we infer the size of a sprite from the shape of an entity
    for(auto const& visible : comps.visible) {
        Vec2F quad_sz = {0.f, 0.f};
        if(comps.sphere.count(visible.first) > 0) {
            auto const& comp = comps.sphere.at(visible.first);
            quad_sz = glm::max(quad_sz, Vec2F(comp.rad * 2.f));
        } else if(comps.rect.count(visible.first) > 0) {
            auto const& comp = comps.rect.at(visible.first);
            quad_sz = glm::max(quad_sz, comp.sz);
        } else {
            LUX_LOG("entity %u has a visible component, but no shape to"
                    "infer the size from", visible.first);
        }
        net_comps->visible[visible.first] =
            {visible.second.visible_id, quad_sz};
    }
    for(auto const& item : comps.item) {
        net_comps->item[item.first] = {item.second.weight};
    }
    for(auto const& destructible : comps.destructible) {
        net_comps->destructible[destructible.first] =
            {destructible.second.dur, destructible.second.dur_max};
    }
    for(auto const& health : comps.health) {
        net_comps->health[health.first] =
            {health.second.hp, health.second.hp_max};
    }
    for(auto const& container : comps.container) {
        net_comps->container[container.first] = {container.second.items};
    }
    for(auto const& orientation : comps.orientation) {
        net_comps->orientation[orientation.first] = {orientation.second.angle};
    }
}
