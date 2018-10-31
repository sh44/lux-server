#include <cstring>
#define GLM_FORCE_PURE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/fast_square_root.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/rotate_vector.hpp>
//
#include <lux_shared/containers.hpp>
//
#include <db.hpp>
#include <map.hpp>
#include <entity.hpp>

Uns constexpr COLLISION_ITERATIONS = 8;
F32 constexpr MIN_SPEED = 0.01f;
F32 constexpr MAX_SPEED = 1.f;
static EntityComps          comps;
SparseDynArr<Entity>        entities;
EntityComps& entity_comps = comps;

EntityId create_entity() {
    EntityId id = entities.emplace();
    return id;
}

EntityId create_item(const char* name) {
    EntityId id = create_entity();
    SizeT len = std::strlen(name);
    comps.name[id].resize(len);
    std::memcpy(comps.name[id].data(), name, len);
    return id;
}

EntityId create_player() {
    LUX_LOG("creating new player");
    EntityId id           = create_entity();
    comps.pos[id]         = {3, 3};
    comps.vel[id]         = {0, 0};
    comps.shape[id]       = EntityComps::Shape
        {{.rect = {{0.5f, 0.5f}}}, .tag = EntityComps::Shape::RECT};
    comps.visible[id]     = {1};
    comps.orientation[id] = {0.f};
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

static bool point_sphere_intersect(Point const& a, Sphere const& b) {
    return glm::distance(a, b.pos) <= b.rad;
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

static bool point_aabb_intersect(Point const& a, Aabb const& b) {
    Point min = b.pos - b.sz;
    Point max = b.pos + b.sz;
    return a.x >= min.x &&
           a.x <= max.x &&
           a.y >= min.y &&
           a.y <= max.y;
}

static bool point_rect_intersect(Point const& a, Arr<Point, 3> const& bp) {
    for(Uns i = 0; i < 2; ++i) {
        Point side = bp[i + 1] - bp[0];
        F32 p = glm::dot(a - bp[0], side);
        if(p < 0.f || p > glm::dot(side, side)) return false;
    }
    return true;
}

static bool point_rect_intersect(Point const& a, Rect const& b) {
    Arr<Point, 3> bp;
    rect_points(b, {{-1.f, -1.f}, {1.f, -1.f}, {-1.f, 1.f}}, &bp);
    return point_rect_intersect(a, bp);
}

static bool line_line_intersect(Line const& a, Line const& b) {
    //@TODO something slightly off here
    Vec2F ab = a.beg;
    Vec2F bb = b.beg;
    Vec2F ad = a.end - a.beg;
    Vec2F bd = b.end - b.beg;
    Vec2F df = bb - ab;
    F32 t0 = -(df.x * ad.y + -(df.y * ad.x)) / ((bd.x * ad.y) - (ad.x * bd.y));
    F32 t1 = (bd.x * t0 + df.x) / ad.x;
    return (t0 >= 0.f && t0 <= 1.f) &&
           (t1 >= 0.f && t1 <= 1.f);
}

static bool line_sphere_intersect(Line const& a, Sphere const& b) {
    Point ray  = a.end - a.beg;
    Point diff = a.beg - b.pos;
    F32 j = glm::length2(ray);
    F32 k = 2.f * glm::dot(ray, diff);
    F32 l = glm::length2(diff) - std::pow(b.rad, 2);
    F32 delta = std::pow(k, 2) - 4.f * j * l;
    if(delta < 0.f) return false;
    F32 t = ((-k) + std::sqrt(delta)) / (2.f * j);
    ///we don't really care about the second intersection point if it exists
    return t >= 0.f && t <= 1.f;
}

static bool line_aabb_intersect(Line const& a, Aabb const& b) {
    Point bp[4];
    aabb_points(b, {{-1.f, -1.f}, {1.f, -1.f}, {-1.f, 1.f}, {1.f, 1.f}}, &bp);
    for(Uns i = 0; i < 4; ++i) {
        if(line_line_intersect(a, {bp[i], bp[(i + 1) % 4]})) return true;
    }
    return false;
}

static bool line_rect_intersect(Line const& a, Rect const& b) {
    Point bp[4];
    rect_points(b, {{-1.f, -1.f}, {1.f, -1.f}, {-1.f, 1.f}, {1.f, 1.f}}, &bp);
    for(Uns i = 0; i < 4; ++i) {
        if(line_line_intersect(a, {bp[i], bp[(i + 1) % 4]})) return true;
    }
    return false;
}

static bool sphere_sphere_intersect(Sphere const& a, Sphere const& b) {
    return glm::distance(a.pos, b.pos) <= a.rad + b.rad;
}

static bool sphere_aabb_intersect(Sphere const& a, Aabb const& b) {
    F32 sq_dst = 0.0f;
    Vec2F min = b.pos - b.sz;
    Vec2F max = b.pos + b.sz;
    for(Uns i = 0; i < 2; i++) {
        F32 v = a.pos[i];
        if(v < min[i]) sq_dst += (min[i] - v) * (min[i] - v);
        if(v > max[i]) sq_dst += (v - max[i]) * (v - max[i]);
    }
    return sq_dst <= std::pow(a.rad, 2);
}

static bool sphere_rect_intersect(Sphere const& a, Rect const& b) {
    if(point_rect_intersect(a.pos, b)) return true;
    Point bp[4];
    rect_points(b, {{-1.f, -1.f}, {1.f, -1.f}, {-1.f, 1.f}, {1.f, 1.f}}, &bp);
    for(Uns i = 0; i < 4; ++i) {
        if(line_sphere_intersect({bp[i], bp[(i + 1) % 4]}, a)) return true;
    }
    return false;
}

static bool aabb_aabb_intersect(Aabb const& a, Aabb const& b) {
    return glm::all(glm::lessThanEqual(glm::abs(a.pos - b.pos), a.sz + b.sz));
}

static bool aabb_rect_intersect(Aabb const& a, Rect const& b) {
    Vec2F constexpr corners[4] =
        {{-1.f, -1.f}, {-1.f, 1.f}, {1.f, -1.f}, {1.f, 1.f}};
    Arr<Point, 4> ap;
    Arr<Point, 4> bp;
    aabb_points(a, corners, &ap);
    rect_points(b, corners, &bp);
    for(Uns i = 0; i < 4; ++i) {
        if(point_aabb_intersect(bp[i], a)) return true;
        if(point_rect_intersect(ap[i], *(Arr<Point, 3>*)&bp)) return true;
    }
    return false;
}

static bool rect_rect_intersect(Rect const& a, Rect const& b) {
    Vec2F constexpr corners[4] =
        {{-1.f, -1.f}, {-1.f, 1.f}, {1.f, -1.f}, {1.f, 1.f}};
    Arr<Point, 4> ap;
    Arr<Point, 4> bp;
    rect_points(a, corners, &ap);
    rect_points(b, corners, &bp);
    for(Uns i = 0; i < 4; ++i) {
        if(point_rect_intersect(bp[i], *(Arr<Point, 3>*)&ap)) return true;
        if(point_rect_intersect(ap[i], *(Arr<Point, 3>*)&bp)) return true;
    }
    return false;
}

static bool sphere_shape_intersect(Sphere const& a, CollisionShape const& b) {
    switch(b.shape.tag) {
    case EntityComps::Shape::SPHERE: {
        Sphere b_sphere = {b.pos, b.shape.sphere.rad};
        return sphere_sphere_intersect(a, b_sphere);
    }
    case EntityComps::Shape::RECT:
        if(b.angle == 0.f) {
            Aabb b_aabb = {b.pos, b.shape.rect.sz};
            return sphere_aabb_intersect(a, b_aabb);
        } else {
            Rect b_rect = {b.pos, b.shape.rect.sz, b.angle};
            return sphere_rect_intersect(a, b_rect);
        }
    default: LUX_UNREACHABLE();
    }
}

static bool aabb_shape_intersect(Aabb const& a, CollisionShape const& b) {
    switch(b.shape.tag) {
    case EntityComps::Shape::SPHERE: {
        Sphere b_sphere = {b.pos, b.shape.sphere.rad};
        return sphere_aabb_intersect(b_sphere, a);
    }
    case EntityComps::Shape::RECT:
        if(b.angle == 0.f) {
            Aabb b_aabb = {b.pos, b.shape.rect.sz};
            return aabb_aabb_intersect(a, b_aabb);
        } else {
            Rect b_rect = {b.pos, b.shape.rect.sz, b.angle};
            return aabb_rect_intersect(a, b_rect);
        }
    default: LUX_UNREACHABLE();
    }
}

static bool rect_shape_intersect(Rect const& a, CollisionShape const& b) {
    switch(b.shape.tag) {
    case EntityComps::Shape::SPHERE: {
        Sphere b_sphere = {b.pos, b.shape.sphere.rad};
        return sphere_rect_intersect(b_sphere, a);
    }
    case EntityComps::Shape::RECT:
        if(b.angle == 0.f) {
            Aabb b_aabb = {b.pos, b.shape.rect.sz};
            return aabb_rect_intersect(b_aabb, a);
        } else {
            Rect b_rect = {b.pos, b.shape.rect.sz, b.angle};
            return rect_rect_intersect(a, b_rect);
        }
    default: LUX_UNREACHABLE();
    }
}

static bool broad_phase(CollisionShape const& a, CollisionShape const& b) {
    return aabb_aabb_intersect(get_aabb(a), get_aabb(b));
}

static bool shape_shape_intersect(CollisionShape const& a,
                                  CollisionShape const& b) {
    switch(a.shape.tag) {
    case EntityComps::Shape::SPHERE: {
        Sphere a_sphere = {a.pos, a.shape.sphere.rad};
        return sphere_shape_intersect(a_sphere, b);
    }
    case EntityComps::Shape::RECT:
        if(a.angle == 0.f) {
            Aabb a_aabb = {a.pos, a.shape.rect.sz};
            return aabb_shape_intersect(a_aabb, b);
        } else {
            Rect a_rect = {a.pos, a.shape.rect.sz, a.angle};
            return rect_shape_intersect(a_rect, b);
        }
    default: LUX_UNREACHABLE();
    }
}

static bool check_map_collision(CollisionShape const& a) {
    Aabb block_shape = {{}, {0.5f, 0.5f}};
    MapPos map_pos = (MapPos)glm::floor(a.pos);
    //@IMPROVE we might optimize for situations with a single point, we would
    //need to check if the map bound has a chance to cross the tile boundary
    Vec2I bound = glm::ceil(get_aabb_sz(a));
    for(MapCoord y = map_pos.y - bound.y; y <= map_pos.y + bound.y; ++y) {
        for(MapCoord x = map_pos.x - bound.x; x <= map_pos.x + bound.x; ++x) {
            guarantee_chunk(to_chk_pos({x, y}));
            if(is_tile_wall({x, y})) {
                block_shape.pos = Vec2F(x, y) + Vec2F(0.5f, 0.5f);
                if(aabb_shape_intersect(block_shape, a)) return true;
            }
        }
    }
    return false;
}

static EntityId check_entities_collision(CollisionShape const& a, EntityId a_id) {
    ChkPos chk_pos = to_chk_pos(a.pos);
    constexpr ChkPos offsets[9] =
        {{-1, -1}, { 0, -1}, { 1, -1},
         {-1,  0}, { 0,  0}, { 1,  0},
         {-1,  1}, { 0,  1}, { 1,  1}};
    for(auto const& offset : offsets) {
        ChkPos off_pos = chk_pos + offset;
        if(collision_sectors.count(off_pos) > 0) {
            for(auto const& id : collision_sectors.at(off_pos)) {
                if(id == a_id) continue;
                if(comps.pos.count(id) > 0 && comps.shape.count(id) > 0) {
                    auto const& pos = comps.pos.at(id);
                    F32 angle = 0.f;
                    if(comps.orientation.count(id) > 0) {
                        angle = comps.orientation.at(id).angle;
                    }
                    CollisionShape shape = {comps.shape.at(id), angle, pos};
                    if(broad_phase(a, shape) &&
                       shape_shape_intersect(a, shape)) {
                        return id;
                    }
                }
            }
        }
    }
    return entities.end();
}

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
        //reallocation is expensive, so maybe let's keep it around for now
        //@TODO proper cleanup
        /*if(it->size() == 0) {
            it = collision_sectors.erase(it);
        } else ++it;*/
        ++it;
    }
    entities.foreach([](EntityId id) {
        if(comps.pos.count(id) > 0 &&
           comps.vel.count(id) > 0) {
            auto& pos = comps.pos.at(id);
            auto& vel = comps.vel.at(id);
            EntityVec old_pos = pos;
            if(comps.shape.count(id) > 0) {
                collision_sectors[to_chk_pos(old_pos)].insert(id);
                F32 speed = glm::length(vel);
                if(speed < MIN_SPEED) {
                    vel = {0.f, 0.f};
                } else {
                    if(speed > MAX_SPEED) {
                        static_assert(MAX_SPEED != 0.f);
                        vel = glm::normalize(vel) * MAX_SPEED;
                    }
                    F32 angle = 0.f;
                    if(comps.orientation.count(id) > 0) {
                        angle = comps.orientation.at(id).angle;
                    }
                    Vec2F try_vel = vel;
                    for(Uns i = 0; i < COLLISION_ITERATIONS; i++) {
                        EntityVec new_pos = pos + try_vel;
                        CollisionShape shape = {comps.shape.at(id),
                                                angle, new_pos};
                        EntityId col = check_entities_collision(shape, id);
                        if(col != entities.end()) {
                            if(comps.vel.count(col) > 0) {
                                comps.vel.at(col) =
                                    (comps.vel.at(col) + vel) / 2.f;
                            }
                            if(comps.container.count(id) > 0 &&
                               comps.item.count(col) > 0) {
                                comps.pos.erase(col);
                                comps.container.at(id).items.emplace_back(col);
                            }
                        } else if(!check_map_collision(shape)) {
                            pos = new_pos;
                            break;
                        }
                        try_vel /= 2.f;
                    }
                    if(to_chk_pos(old_pos) != to_chk_pos(pos)) {
                        collision_sectors.at(to_chk_pos(old_pos)).erase(id);
                        collision_sectors[to_chk_pos(pos)].insert(id);
                    }
                }
            } else {
                pos += vel;
            }
            //@TODO magic number
            vel *= 0.9f;
        }
    });
    entities.free_slots();
}

void remove_entity(EntityId entity) {
    LUX_ASSERT(entities.contains(entity));
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
        net_comps->orientation[orientation.first] = {orientation.second.angle};
    }
}

void entity_rotate_to(EntityId id, F32 angle) {
    //@TODO we probably should apply angular momentum or something simillar
    LUX_ASSERT(entities.contains(id));
    if(comps.orientation.count(id) > 0) {
        if(comps.pos.count(id) > 0 &&
           comps.shape.count(id) > 0) {
            CollisionShape shape = {comps.shape.at(id), angle,
                                    comps.pos.at(id)};
            if(check_entities_collision(shape, id) == entities.end() &&
               !check_map_collision(shape)) {
                comps.orientation.at(id).angle = angle;
            }
        } else {
            comps.orientation.at(id).angle = angle;
        }
    }
}
