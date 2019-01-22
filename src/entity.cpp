#include <cmath>
#include <cstring>
#define GLM_FORCE_PURE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/fast_square_root.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/euler_angles.hpp>
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

static RasenExtEnv entity_env;

EntityId create_entity() {
    EntityId id = entities.emplace();
    LUX_LOG("created entity #%u", id);
    return id;
}

EntityId create_item(Str name) {
    EntityId id = create_entity();
    comps.name[id] = name;
    return id;
}

LUX_MAY_FAIL static entity_break_block(RasenFrame* frame) {
    EntityId id;
    for(Uns i = 0; i < sizeof(EntityId); ++i) {
        LUX_RETHROW(frame->read_sys_mem(((U8*)&id) + i, 0 + i),
                    "failed to read entity id");
    }
    LUX_RETHROW(comps.orientation.count(id) > 0,
                "entity has no orientation component");
    LUX_RETHROW(comps.pos.count(id) > 0,
                "entity has no position component");
    Vec2F rot = comps.orientation.at(id).angles;
    Vec3F src = comps.pos.at(id) + Vec3F(0, 0, 3.6);
    Vec3F dir;
    dir.x = cos(rot.y) * sin(-rot.x);
    dir.y = cos(rot.y) * cos(-rot.x);
    dir.z = sin(rot.y);
    dir = normalize(dir) * 10.f;
    MapPos hit_pos;
    if(map_cast_ray_interior(&hit_pos, src, src + dir)) {
        Block& block = write_block(hit_pos);
        if(block.lvl > 0x0f) {
            block.lvl -= 0xf;
        } else {
            block.lvl = 0x00;
            block.id = void_block;
        }
    }
    return LUX_OK;
}

LUX_MAY_FAIL static entity_place_block(RasenFrame* frame) {
    EntityId id;
    for(Uns i = 0; i < sizeof(EntityId); ++i) {
        LUX_RETHROW(frame->read_sys_mem(((U8*)&id) + i, 0 + i),
                    "failed to read entity id");
    }
    LUX_RETHROW(comps.orientation.count(id) > 0,
                "entity has no orientation component");
    LUX_RETHROW(comps.pos.count(id) > 0,
                "entity has no position component");
    Vec2F rot = comps.orientation.at(id).angles;
    Vec3F src = comps.pos.at(id) + Vec3F(0, 0, 3.6);
    Vec3F dir;
    dir.x = cos(rot.y) * sin(-rot.x);
    dir.y = cos(rot.y) * cos(-rot.x);
    dir.z = sin(rot.y);
    dir = normalize(dir) * 10.f;
    MapPos hit_pos;
    static BlockId stone = db_block_id("stone_wall"_l);
    if(map_cast_ray_exterior(&hit_pos, src, src + dir)) {
        Block& block = write_block(hit_pos);
        if(block.id != stone) {
            block.id  = stone;
            block.lvl = 0x00;
        }
        if(block.lvl < 0xf0) {
            block.lvl += 0xf;
        } else block.lvl = 0xff;
    }
    return LUX_OK;
}

LUX_MAY_FAIL static entity_move(RasenFrame* frame) {
    EntityId id;
    for(Uns i = 0; i < sizeof(EntityId); ++i) {
        LUX_RETHROW(frame->read_sys_mem(((U8*)&id) + i, 0 + i),
                    "failed to read entity id");
    }
    Vec3<I8> dir;
    LUX_RETHROW(comps.vel.count(id) > 0,
                "entity has no linear velocity component");
    LUX_RETHROW(frame->pop((U8*)&dir.x),
                "failed to read X direction");
    LUX_RETHROW(frame->pop((U8*)&dir.y),
                "failed to read Y direction");
    dir.z = 0;
    comps.vel.at(id) += (Vec3F)dir * ENTITY_L_VEL;
    return LUX_OK;
}

LUX_MAY_FAIL static entity_rotate(RasenFrame* frame) {
    EntityId id;
    for(Uns i = 0; i < sizeof(EntityId); ++i) {
        LUX_RETHROW(frame->read_sys_mem(((U8*)&id) + i, 0 + i),
                    "failed to read entity id");
    }
    Vec2<I8> dir;
    LUX_RETHROW(comps.orientation.count(id) > 0,
                "entity has no angular velocity component");
    LUX_RETHROW(frame->pop((U8*)&dir.x),
                "failed to read yaw");
    LUX_RETHROW(frame->pop((U8*)&dir.y),
                "failed to read pitch");
    Vec2F f_dir = (Vec2F)dir / 128.f;
    comps.orientation.at(id).angles = f_dir * (tau / 2.f);
    return LUX_OK;
}

EntityId create_player() {
    LUX_LOG("creating new player");
    EntityId id            = create_entity();
    comps.pos[id]          = {300, 300, 0};
    comps.vel[id]          = {0, 0, 0};
    comps.shape[id]        = EntityComps::Shape
        {{.box = {{0.8f, 0.5f, 1.f}}}, .tag = EntityComps::Shape::BOX};
    comps.visible[id]      = {2};
    comps.orientation[id]  = {{0.f, 0.f, 0.f}, {0.f, 0.f}};
    comps.a_vel[id]        = {{0.f, 0.f}, {0.15f, 0.15f}};
    entity_env.register_func("entity_move"_l, &entity_move);
    entity_env.register_func("entity_rotate"_l, &entity_rotate);
    entity_env.register_func("entity_break_block"_l, &entity_break_block);
    entity_env.register_func("entity_place_block"_l, &entity_place_block);
    auto& ai = comps.ai[id];
    ai.rn_env.add_external_parent(entity_env);
    /*EntityId limb_id       = create_entity();
    comps.pos[limb_id]     = {-1.15f, 0, 0};
    comps.shape[limb_id]   = EntityComps::Shape
        {{.box = {{0.4f, 0.25f, 0.1f}}}, .tag = EntityComps::Shape::BOX};
    comps.visible[limb_id] = {1};
    comps.parent[limb_id] = id;
    comps.orientation[limb_id] = {{0.5f, 0.f}, 0.f};
    comps.a_vel[limb_id]    = {-0.08f, 0.f};
    EntityId bob       = create_entity();
    comps.pos[bob]     = {-1.0f, 0, 0};
    comps.shape[bob]   = EntityComps::Shape
        {{.box = {{0.6f, 0.2f, 0.1f}}}, .tag = EntityComps::Shape::BOX};
    comps.visible[bob] = {1};
    comps.parent[bob] = limb_id;
    comps.orientation[bob] = {{0.6f, 0.f}, 0.f};
    comps.a_vel[bob]    = {-0.06f, 0.f};*/
    return id;
}

static VecMap<ChkPos, IdSet<EntityId>> collision_sectors;

typedef EntityVec Point;

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
            if(ai.active && ai.rn_env.symbol_table.count("tick"_l) > 0) {
                U8 s[RN_STACK_LEN];
                U8 sp = 0;
                (void)entity_do_action(id, ai.rn_env.symbol_table["tick"_l],
                    Slice<U8>{s, sp});
            }
        }
        if(comps.orientation.count(id) > 0 &&
           comps.a_vel.count(id) > 0) {
            comps.orientation.at(id).angles += comps.a_vel.at(id).vel;
            comps.a_vel.at(id).vel *= 1.f - comps.a_vel.at(id).damping;
            //@TODO divide to prevent precision loss or something?
            //@TODO Modular arithmetic class
        }
        if(comps.pos.count(id) > 0 &&
           comps.vel.count(id) > 0) {
            auto& pos = comps.pos.at(id);
            EntityVec& vel = comps.vel.at(id);
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
                Vec2F angles = comps.orientation.at(id).angles;
                glm::mat4 bob = glm::eulerAngleZX(angles.x, angles.y);
                pos += (Vec3F)(bob * Vec4F(vel, 1.f));
                guarantee_chunk(to_chk_pos(floor(pos)));
                /*while(get_block(floor(pos)).id == void_block) {
                    pos.z -= 1;
                    guarantee_chunk(to_chk_pos(floor(pos)));
                }
                while(get_block(floor(pos)).id != void_block) {
                    pos.z += 1;
                    guarantee_chunk(to_chk_pos(floor(pos)));
                }*/
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
    for(auto const& visible : comps.visible) {
        net_comps->visible[visible.first] =
            {visible.second.visible_id};
    }
    for(auto const& orientation : comps.orientation) {
        net_comps->orientation[orientation.first] =
            {orientation.second.origin, orientation.second.angles};
    }
    for(auto const& parent : comps.parent) {
        net_comps->parent[parent.first] = parent.second;
    }
}

LUX_MAY_FAIL entity_do_action(EntityId entity_id, U16 action_id,
                              Slice<U8> const& stack) {
    LUX_ASSERT(comps.ai.count(entity_id) > 0);
    auto& ai = comps.ai.at(entity_id);
    LUX_ASSERT(ai.rn_env.available_funcs.len > action_id);
    U8 sys_mem[0x10];
    static_assert(sizeof(sys_mem) >= sizeof(EntityId));
    std::memcpy(sys_mem, &entity_id, sizeof(entity_id));
    RasenFrame frame = {stack, (Slice<U8>)sys_mem};
    return ai.rn_env.call(action_id, &frame);
}
