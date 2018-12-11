#include <cmath>
#include <cstring>
#define GLM_FORCE_PURE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/fast_square_root.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/rotate_vector.hpp>
//
#include <lux_shared/common.hpp>
//
#include <db.hpp>
#include <map.hpp>
#include <entity.hpp>
#include <server.hpp>

F32 constexpr VEL_DAMPING = 0.9f;
F32 constexpr MIN_SPEED = 0.01f;
F32 constexpr MAX_SPEED = 1.f;
static EntityComps          comps;
EntityComps& entity_comps = comps;
SparseDynArr<Entity>        entities;

EntityId create_entity() {
    EntityId id = entities.emplace();
    return id;
}

EntityId create_item(Str name) {
    EntityId id = create_entity();
    comps.name[id] = name;
    return id;
}

EntityId create_player() {
    LUX_LOG("creating new player");
    EntityId id            = create_entity();
    comps.pos[id]          = {3, 3};
    comps.vel[id]          = {0, 0};
    comps.shape[id]        = EntityComps::Shape
        {{.rect = {{0.8f, 0.5f}}}, .tag = EntityComps::Shape::RECT};
    comps.visible[id]      = {2};
    comps.orientation[id]  = {{0.f, 0.f}, 0.f};
    comps.a_vel[id]        = {0.f, 0.15f};
    EntityId limb_id       = create_entity();
    comps.pos[limb_id]     = {-1.15f, 0};
    comps.shape[limb_id]   = EntityComps::Shape
        {{.rect = {{0.4f, 0.25f}}}, .tag = EntityComps::Shape::RECT};
    comps.visible[limb_id] = {1};
    comps.parent[limb_id] = id;
    comps.orientation[limb_id] = {{0.5f, 0.f}, 0.f};
    comps.a_vel[limb_id]    = {-0.08f, 0.f};
    EntityId bob       = create_entity();
    comps.pos[bob]     = {-1.0f, 0};
    comps.shape[bob]   = EntityComps::Shape
        {{.rect = {{0.6f, 0.2f}}}, .tag = EntityComps::Shape::RECT};
    comps.visible[bob] = {1};
    comps.parent[bob] = limb_id;
    comps.orientation[bob] = {{0.6f, 0.f}, 0.f};
    comps.a_vel[bob]    = {-0.06f, 0.f};
    return id;
}

static VecMap<ChkPos, SortSet<EntityId>> collision_sectors;

struct CollisionShape {
    EntityComps::Shape shape;
    F32 angle;
    Vec2F pos;
};

typedef EntityVec Point;

struct Line {
    Point beg;
    Point end;
};

struct Sphere {
    Point pos;
    F32   rad;
};

struct Aabb {
    Point pos;
    Vec2F sz;
};

struct Rect {
    Point pos;
    Vec2F sz;
    F32   angle;
};

static Vec2F get_aabb_sz(CollisionShape const& a) {
    switch(a.shape.tag) {
        case EntityComps::Shape::SPHERE:
            return Vec2F(a.shape.sphere.rad);
        case EntityComps::Shape::RECT:
            return Vec2F(glm::length(a.shape.rect.sz));
        default: LUX_UNREACHABLE();
    }
}

static Aabb get_aabb(CollisionShape const& a) {
    return {a.pos, get_aabb_sz(a)};
}

static Vec2F rect_point(Rect const& a, Vec2F point) {
    return glm::rotate(point * a.sz, a.angle) + a.pos;
}

template<SizeT len>
static void rect_points(Rect const& a,
                        Arr<Point, len> const& in, Arr<Point, len>* out) {
    F32 c = std::cos(a.angle);
    F32 s = std::sin(a.angle);
    glm::mat2 rot = {c, -s,
                     s,  c};
    for(Uns i = 0; i < len; ++i) {
        Point vert = in[i] * a.sz;
        (*out)[i] = vert * rot + a.pos;
    }
}

template<SizeT len>
static void aabb_points(Aabb const& a,
                        Arr<Point, len> const& in, Arr<Point, len>* out) {
    for(Uns i = 0; i < len; ++i) {
        (*out)[i] = in[i] * a.sz + a.pos;
    }
}

static bool broad_phase(CollisionShape const& a, CollisionShape const& b) {
    return false;//aabb_aabb_intersect(get_aabb(a), get_aabb(b));
}

/*
static F32 move_into_map(CollisionShape const& a, Vec2F vel, Vec2F* slide) {
    Rect block_shape = {{}, {0.5f, 0.5f}, 0.f};
    MapPos map_pos = (MapPos)glm::floor(a.pos);
    //@TODO the "bound" needs to be raycasted and take speed into the account,
    //so that small objects might only have to check one block for collision,
    //and high-speed objects won't pass through walls
    Vec2I bound = glm::ceil(get_aabb_sz(a));
    //@TODO other shapes
    Rect ar = {a.pos, a.shape.rect.sz, a.angle};
    F32 mult = 1.f;
    *slide = {0.f, 0.f};
    for(MapCoord y = map_pos.y - bound.y; y <= map_pos.y + bound.y; ++y) {
        for(MapCoord x = map_pos.x - bound.x; x <= map_pos.x + bound.x; ++x) {
            guarantee_chunk(to_chk_pos({x, y}));
            if(is_tile_wall({x, y})) {
                block_shape.pos = Vec2F(x, y) + Vec2F(0.5f, 0.5f);
                Vec2F slide_temp0;
                Vec2F slide_temp1;
                Vec2F slide_temp;
                F32 mult_temp0 = move_into(ar, block_shape,  vel, &slide_temp0);
                F32 mult_temp1 = 1.f;//move_into(block_shape, ar, -vel, &slide_temp1);
                F32 mult_temp;
                if(mult_temp0 < mult_temp1) {
                    slide_temp = slide_temp0;
                    mult_temp = mult_temp0;
                } else {
                    slide_temp = -slide_temp1;
                    mult_temp = mult_temp1;
                }
                if(mult_temp < mult) {
                    *slide = slide_temp;
                    mult = mult_temp;
                }
            }
        }
    }
    return mult;
}
*/

void entities_tick() {
    //@CONSIDER moving into the main loop
    for(auto it = collision_sectors.begin(), end = collision_sectors.end();
             it != end;) {
        for(auto its = it->second.begin(), ends = it->second.end();
                 its != ends;) {
            if(comps.pos.count(*its) == 0) {
                its = it->second.erase(its);
            } else ++its;
        }
        if(it->second.size() == 0) {
            it = collision_sectors.erase(it);
        } else ++it;
    }
    for(auto pair : entities) {
        EntityId id = pair.k;
        if(comps.parent.count(id) > 0) {
            EntityId parent = comps.parent.at(id);
            if(!entities.contains(parent)) {
                LUX_LOG_WARN("stray child entity #%u", id);
                LUX_LOG_WARN("\tits parent #%u no longer exists", parent)
                LUX_LOG_WARN("\tdeleting the child, it should have been"
                             " deleted along with the parent");
                remove_entity(id);
                continue;
            }
        }
        if(comps.ai.count(id) > 0) {
            auto& ai = comps.ai.at(id);
            if(ai.active && ai.rn_env.funcs_lookup.count("tick"_l) > 0) {
                U8 s[RN_STACK_LEN];
                U8 sp = 0;
                ai.rn_env.call("tick"_l, RasenStack{&s, &sp});
            }
        }
        if(comps.orientation.count(id) > 0 &&
           comps.a_vel.count(id) > 0) {
            comps.orientation.at(id).angle += comps.a_vel.at(id).vel;
            comps.a_vel.at(id).vel *= 1.f - comps.a_vel.at(id).damping;
            //@TODO divide to prevent precision loss or something?
            //@TODO Modular arithmetic class
        }
        if(comps.pos.count(id) > 0 &&
           comps.vel.count(id) > 0) {
            auto& pos = comps.pos.at(id);
            auto& vel = comps.vel.at(id);
            EntityVec old_pos = pos;
            /*if(comps.shape.count(id) > 0) {
                collision_sectors[to_chk_pos(old_pos)].insert(id);
                F32 speed = glm::length(vel);
                if(speed < MIN_SPEED) {
                    vel = {0.f, 0.f};
                } else if(speed > MAX_SPEED) {
                    vel = glm::normalize(vel) * MAX_SPEED;
                }
                F32 angle = 0.f;
                if(comps.orientation.count(id) > 0) {
                    angle = comps.orientation.at(id).angle;
                }
                CollisionShape shape = {
                    comps.shape.at(id),
                    angle, pos
                };
                if(to_chk_pos(old_pos) != to_chk_pos(pos)) {
                    collision_sectors.at(to_chk_pos(old_pos)).erase(id);
                    collision_sectors[to_chk_pos(pos)].insert(id);
                }
            } else {*/
                pos += vel;
            //}
            vel *= VEL_DAMPING;
        }
    }
    entities.free_slots();
}

void remove_entity(EntityId entity) {
    LUX_ASSERT(entities.contains(entity));
    LUX_LOG("removing entity %u", entity);
    entities.erase(entity);
    ///this is only gonna grow longer...
    if(comps.pos.count(entity)         > 0) comps.pos.erase(entity);
    if(comps.vel.count(entity)         > 0) comps.vel.erase(entity);
    if(comps.name.count(entity)        > 0) comps.name.erase(entity);
    if(comps.shape.count(entity)       > 0) comps.shape.erase(entity);
    if(comps.visible.count(entity)     > 0) comps.visible.erase(entity);
    if(comps.item.count(entity)        > 0) comps.item.erase(entity);
    if(comps.container.count(entity)   > 0) comps.container.erase(entity);
    if(comps.orientation.count(entity) > 0) comps.orientation.erase(entity);
    if(comps.ai.count(entity)          > 0) comps.ai.erase(entity);
}

void get_net_entity_comps(NetSsTick::EntityComps* net_comps) {
    //@TODO figure out a way to reduce the verbosity of this function
    for(auto const& pos : comps.pos) {
        net_comps->pos[pos.first] = pos.second;
    }
    for(auto const& name : comps.name) {
        net_comps->name[name.first] = (Str)name.second;
    }
    ///we infer the size of a sprite from the shape of an entity
    for(auto const& visible : comps.visible) {
        Vec2F quad_sz = {0.f, 0.f};
        if(comps.shape.count(visible.first) > 0) {
            auto const& shape = comps.shape.at(visible.first);
            switch(shape.tag) {
                case EntityComps::Shape::SPHERE: {
                    quad_sz = Vec2F(shape.sphere.rad);
                } break;
                case EntityComps::Shape::RECT: {
                    quad_sz = shape.rect.sz;
                } break;
                default: LUX_UNREACHABLE();
            }
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
    for(auto const& container : comps.container) {
        net_comps->container[container.first] = {container.second.items};
    }
    for(auto const& orientation : comps.orientation) {
        net_comps->orientation[orientation.first] =
            {orientation.second.origin, orientation.second.angle};
    }
    for(auto const& parent : comps.parent) {
        net_comps->parent[parent.first] = parent.second;
    }
}
