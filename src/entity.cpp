#define GLM_FORCE_PURE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/fast_square_root.hpp>
#include <glm/gtx/component_wise.hpp>
//
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
    comps.pos[id]         = {2, 2};
    comps.vel[id]         = {0, 0};
    comps.sphere[id]      = {0.8f};
    comps.visible[id]     = {0};
    comps.orientation[id] = {0.f};
    return id;
}

struct CollisionSphere {
    EntityComps::Sphere const& shape;
    Vec2F               const& pos;
};
struct CollisionAabb {
    EntityComps::Rect const& shape;
    Vec2F             const& pos;
};
struct CollisionRect {
    EntityComps::Rect        const& shape;
    Vec2F                    const& pos;
    EntityComps::Orientation const& orientation;
};

static bool broad_phase(EntityVec const& a, EntityVec const& b) {
    constexpr F32 max_distance = (F32)CHK_SIZE;
    return glm::fastDistance(a, b) <= max_distance;
}

static bool narrow_phase(CollisionSphere const& a,
                         CollisionSphere const& b) {
    return glm::distance(a.pos, b.pos) <= a.shape.rad + b.shape.rad;
}

static bool narrow_phase(CollisionSphere const& a,
                         CollisionAabb   const& b) {
    F32 sq_dst = 0.0f;
    Vec2F min = {b.pos.x, b.pos.y};
    Vec2F max = min + b.shape.sz;
    for(Uns i = 0; i < 2; i++) {
        float v = Vec2F(a.pos)[i];
        if(v < min[i]) sq_dst += (min[i] - v) * (min[i] - v);
        if(v > max[i]) sq_dst += (v - max[i]) * (v - max[i]);
    }
    return sq_dst <= a.shape.rad;
}

static MapCoord get_map_bound(CollisionSphere const& a) {
    return glm::ceil(a.shape.rad);
}

static MapCoord get_map_bound(CollisionAabb const& a) {
    return glm::ceil(glm::compMax(a.shape.sz));
}

template<typename A>
static bool check_map_collision(A const& a) {
    constexpr EntityComps::Rect block_shape = {{1.f, 1.f}};
    MapPos map_pos = (MapPos)glm::floor(a.pos);
    MapCoord bound = get_map_bound(a);
    for(MapCoord y = map_pos.y - bound; y <= map_pos.y + bound; ++y) {
        for(MapCoord x = map_pos.x - bound; x <= map_pos.x + bound; ++x) {
            guarantee_chunk(to_chk_pos({x, y}));
            VoxelType const& vox = get_voxel_type({x, y});
            Vec2F h_pos = {x, y};
            if(vox.shape == VoxelType::BLOCK) {
                CollisionAabb block_aabb = {block_shape, h_pos};
                if(narrow_phase(a, block_aabb)) return true;
            }
        }
    }
    return false;
}

template<typename A>
static bool check_entities_collision(A const& a, EntityHandle a_id) {
    //@IMPROVE very slow! O(n^2)!
    //perhaps check neighboring chunks only
    for(auto it = entities.begin(); it != entities.end(); ++it) {
        auto const& id = it.idx;
        if(id == a_id) continue;
        if(comps.pos.count(id) > 0) {
            if(broad_phase(comps.pos.at(id), a.pos)) {
                if(comps.sphere.count(id) > 0) {
                    CollisionSphere sphere = {
                        comps.sphere.at(id),
                        comps.pos.at(id),
                    };
                    if(narrow_phase(a, sphere)) return true;
                }
                if(comps.rect.count(id) > 0) {
                    CollisionAabb aabb = {
                        comps.rect.at(id),
                        comps.pos.at(id),
                    };
                    if(narrow_phase(a, aabb)) return true;
                    //TODO orientation
                }
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
            constexpr ChkPos offsets[] =
                {{0, 0}, {-1, 0}, {1, 0}, {0, -1}, {0, 1}};
            for(auto const& offset : offsets) {
                guarantee_chunk(chk_pos + offset);
            }
            if(comps.vel.count(id) > 0) {
                auto& vel = comps.vel.at(id);
                EntityVec old_pos = pos;
                EntityVec new_pos = pos + vel;
                if(comps.sphere.count(id) > 0) {
                    Vec2F h_pos = (Vec2F)old_pos;
                    CollisionSphere sphere = {comps.sphere.at(id), h_pos};
                    h_pos = {new_pos.x, old_pos.y};
                    //@IMPROVE, don't check for entity-entity collision twice
                    if(!check_map_collision(sphere) &&
                       !check_entities_collision(sphere, id)) {
                        pos.x = new_pos.x;
                    }
                    h_pos = {old_pos.x, new_pos.y};
                    if(!check_map_collision(sphere) &&
                       !check_entities_collision(sphere, id)) {
                        pos.y = new_pos.y;
                    }
                } else {
                    pos += vel;
                }
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
