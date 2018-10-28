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

struct Line {
    EntityVec beg;
    EntityVec end;
};

static F32 get_map_bound(CollisionShape const& a) {
    switch(a.shape.tag) {
        case EntityComps::Shape::SPHERE: return a.shape.sphere.rad;
        case EntityComps::Shape::RECT:
            return glm::length(Vec2F(a.shape.rect.sz.x, a.shape.rect.sz.y));
        default: LUX_UNREACHABLE();
    }
}

static bool broad_phase(CollisionShape const& a,
                        CollisionShape const& b) {
    return glm::distance(a.pos, b.pos) <= get_map_bound(a) + get_map_bound(b);
}

static bool point_point_intersect(EntityVec const&,
                                  CollisionShape const&) {
    return false;
}

static bool point_line_intersect(EntityVec const&,
                                 CollisionShape const&) {
    return false;
}

static bool point_sphere_intersect(EntityVec const& a,
                                   CollisionShape const& b) {
    return glm::distance(a, b.pos) <= b.shape.sphere.rad;
}

static Vec2F rect_point(CollisionShape const& a, Vec2F point) {
    return glm::rotate(point * a.shape.rect.sz, a.angle) + a.pos;
}

static bool point_rect_intersect(EntityVec const& a,
                                 CollisionShape const& b) {
    if(b.angle == 0.f) {
        return a.x >= b.pos.x - b.shape.rect.sz.x &&
               a.x <= b.pos.x + b.shape.rect.sz.x &&
               a.y >= b.pos.y - b.shape.rect.sz.y &&
               a.y <= b.pos.y + b.shape.rect.sz.y;
    } else {
        EntityVec b00 = rect_point(b, {-1.f, -1.f});
        EntityVec b10 = rect_point(b, { 1.f, -1.f});
        EntityVec b01 = rect_point(b, {-1.f,  1.f});
        F32 p0 = glm::dot(b00 - a, b00 - b10);
        F32 p1 = glm::dot(b00 - a, b00 - b01);
        return p0 > 0.f && p0 < glm::dot(b00 - b10, b00 - b10) &&
               p1 > 0.f && p1 < glm::dot(b00 - b01, b00 - b01);
    }
}

static bool line_line_intersect(Line const& a, Line const& b) {
    ///probably slow?
    Vec2F ab = a.beg;
    Vec2F ae = a.end;
    Vec2F bb = b.beg;
    Vec2F be = b.end;
    F32 t0 = (-(ae.y - ab.y) * (ab.x - bb.x) + (ae.x - ab.x) * (ab.y - bb.y)) /
             (-(be.x - bb.x) * (ae.y - ab.y) + (ae.x - ab.x) * (be.y - bb.y));
    F32 t1 = ( (be.x - bb.x) * (ab.y - bb.y) - (be.y - bb.y) * (ab.x - bb.x)) /
             (-(be.x - bb.x) * (ae.y - ab.y) + (ae.x - ab.x) * (be.y - bb.y));
    /*F32 t0 = ((ab.x - bb.x) * (bb.y - be.y) - (ab.y - bb.y) * (bb.x - be.x)) /
             ((ab.x - ae.x) * (bb.y - be.y) - (ab.y - ae.y) * (bb.x - be.x));
    F32 t1 = -((ab.x - ae.x) * (ab.y - bb.y) - (ab.y - ae.y) * (ab.x - bb.x)) /
              ((ab.x - ae.x) * (bb.y - be.y) - (ab.y - ae.y) * (bb.x - be.x));*/
    return (t0 >= 0.f && t0 <= 1.f) &&
           (t1 >= 0.f && t1 <= 1.f);
}

static bool line_sphere_intersect(Line const& a,
                                  CollisionShape const& b) {
    //@TODO might not work
    Vec2F ray = a.end - a.beg;
    Vec2F d = a.beg - b.pos;
    F32 k = 2.f * (ray.x * d.x + ray.y * d.y);
    F32 l = glm::length2(d) - std::pow(b.shape.sphere.rad, 2);
    F32 delta = std::pow(k, 2) - 4.f * l;
    if(delta < 0.f) return false;
    F32 t0 = ((-k) - std::sqrt(delta)) / 2.f;
    F32 t1 = ((-k) + std::sqrt(delta)) / 2.f;
    return (t0 >= 0.f && t0 <= 1.f) ||
           (t1 >= 0.f && t1 <= 1.f);
}

static bool line_rect_intersect(Line const& a,
                                CollisionShape const& b) {
    EntityVec b00 = rect_point(b, {-1.f, -1.f});
    EntityVec b10 = rect_point(b, { 1.f, -1.f});
    EntityVec b01 = rect_point(b, {-1.f,  1.f});
    EntityVec b11 = rect_point(b, { 1.f,  1.f});
    Line d0 = {b00, b10};
    Line d1 = {b10, b11};
    Line d2 = {b11, b01};
    Line d3 = {b01, b00};
    return line_line_intersect(a, d0) || line_line_intersect(a, d1) ||
           line_line_intersect(a, d2) || line_line_intersect(a, d3);
}

static bool sphere_sphere_intersect(CollisionShape const& a,
                                    CollisionShape const& b) {
    return glm::distance(a.pos, b.pos) <=
        a.shape.sphere.rad + b.shape.sphere.rad;
}

static bool sphere_rect_intersect(CollisionShape const& a,
                                  CollisionShape const& b) {
    if(b.angle == 0.f) {
        F32 sq_dst = 0.0f;
        Vec2F min = b.pos - b.shape.rect.sz;
        Vec2F max = b.pos + b.shape.rect.sz;
        for(Uns i = 0; i < 2; i++) {
            F32 v = a.pos[i];
            if(v < min[i]) sq_dst += (min[i] - v) * (min[i] - v);
            if(v > max[i]) sq_dst += (v - max[i]) * (v - max[i]);
        }
        return sq_dst <= std::pow(a.shape.sphere.rad, 2);
    } else {
        EntityVec b00 = rect_point(b, {-1.f, -1.f});
        EntityVec b10 = rect_point(b, { 1.f, -1.f});
        EntityVec b01 = rect_point(b, {-1.f,  1.f});
        EntityVec b11 = rect_point(b, { 1.f,  1.f});
        Line jk = {b00, b10};
        Line kl = {b10, b11};
        Line lm = {b11, b01};
        Line mj = {b01, b00};
        EntityVec center = a.pos;
        return point_rect_intersect(center, b) ||
            line_sphere_intersect(jk, a) || line_sphere_intersect(kl, a) ||
            line_sphere_intersect(lm, a) || line_sphere_intersect(mj, a);
    }
}

static bool rect_rect_intersect(CollisionShape const& a,
                                CollisionShape const& b) {
    if(a.angle == 0.f && b.angle == 0.f) {
        //@TODO dunno if this works at all
        if(glm::any(glm::greaterThan(glm::abs(a.pos - b.pos),
               (a.shape.rect.sz + b.shape.rect.sz) / 2.f))) {
            return false;
        }
        return true;
    } else {
        if(point_rect_intersect(a.pos, b) ||
           point_rect_intersect(b.pos, a)) return true;
        Vec2F constexpr corners[4] =
            {{-1.f, -1.f}, {-1.f, 1.f}, {1.f, 1.f}, {1.f, -1.f}};
        //@TODO this must be soooooo slooow
        for(Uns i = 0; i < 4; ++i) {
            Vec2F ip0 = rect_point(a, corners[i]);
            Vec2F ip1 = rect_point(a, corners[(i + 1) % 4]);
            Line ie = {ip0, ip1};
            for(Uns j = 0; j < 4; ++j) {
                Vec2F jp0 = rect_point(b, corners[j]);
                Vec2F jp1 = rect_point(b, corners[(j + 1) % 4]);
                Line je = {jp0, jp1};
                if(line_line_intersect(ie, je)) return true;
            }
        }
        return false;
    }
}

static bool narrow_phase(CollisionShape const& a, CollisionShape const& b) {
    CollisionShape const& first  = a.shape.tag < b.shape.tag ? a : b;
    CollisionShape const& second = a.shape.tag < b.shape.tag ? b : a;
    switch(first.shape.tag) {
        case EntityComps::Shape::SPHERE: switch(second.shape.tag) {
            case EntityComps::Shape::SPHERE:
                return sphere_sphere_intersect(first, second);
            case EntityComps::Shape::RECT:
                return sphere_rect_intersect(first, second);
            default: LUX_UNREACHABLE();
        }
        case EntityComps::Shape::RECT:
            LUX_ASSERT(second.shape.tag == EntityComps::Shape::RECT);
            return rect_rect_intersect(first, second);
        default: LUX_UNREACHABLE();
    }
    LUX_UNREACHABLE();
}

static bool check_map_collision(CollisionShape const& a) {
    CollisionShape block_shape =
        {{{.rect = {{0.5f, 0.5f}}}, EntityComps::Shape::RECT}, 0.f, {0.5f, 0.5f}};
    MapPos map_pos = (MapPos)glm::floor(a.pos);
    //@IMPROVE we might optimize for situations with a single point, we would
    //need to check if the map bound has a chance to cross the tile boundary
    MapCoord bound = std::ceil(get_map_bound(a));
    for(MapCoord y = map_pos.y - bound; y <= map_pos.y + bound; ++y) {
        for(MapCoord x = map_pos.x - bound; x <= map_pos.x + bound; ++x) {
            guarantee_chunk(to_chk_pos({x, y}));
            VoxelType const& vox = get_voxel_type({x, y});
            if(vox.shape == VoxelType::BLOCK) {
                block_shape.pos = Vec2F(0.5f, 0.5f) + Vec2F(x, y);
                switch(a.shape.tag) {
                    case EntityComps::Shape::SPHERE:
                        if(sphere_rect_intersect(a, block_shape)) {
                            return true;
                        } break;
                    case EntityComps::Shape::RECT:
                        if(rect_rect_intersect(a, block_shape)) {
                            return true;
                        } break;
                    default: LUX_UNREACHABLE();
                }
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
                    if(broad_phase(a, shape) && narrow_phase(a, shape)) {
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
        if(comps.pos.count(id) > 0) {
            auto& pos = comps.pos.at(id);
            ChkPos chk_pos = to_chk_pos(pos);
            constexpr ChkPos offsets[] =
                {{0, 0}, {-1, 0}, {1, 0}, {0, -1}, {0, 1}};
            for(auto const& offset : offsets) {
                //guarantee_chunk(chk_pos + offset);
            }
            if(comps.vel.count(id) > 0) {
                auto& vel = comps.vel.at(id);
                EntityVec old_pos = pos;
                if(comps.shape.count(id) > 0) {
                    collision_sectors[to_chk_pos(old_pos)].insert(id);
                    if(glm::length(vel) < 0.01f) {
                        vel = {0.f, 0.f};
                    } else {
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
                if((MapPos)pos != (MapPos)old_pos) {
                    //del_light_source(old_pos);
                    //add_light_node(pos, {0.5f, 0.5f, 0.5f});
                }
            }
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
