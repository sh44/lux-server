#include <cmath>
#include <cstring>
//
#include <glm/gtx/quaternion.hpp>
//
#include <lux_shared/common.hpp>
//
#include <db.hpp>
#include <map.hpp>
#include <entity.hpp>
#include <server.hpp>

static EntityComps          comps;
EntityComps& entity_comps = comps;
SparseDynArr<Entity>        entities;

EntityId create_entity() {
    EntityId id = entities.emplace();
    LUX_LOG("created entity #%u", id);
    return id;
}

EntityId create_player() {
    LUX_LOG("creating new player");
    EntityId id            = create_entity();
    comps.physics_body[id] = {physics_create_body({0, 0, 80})};
    comps.model[id]      = {0};
    return id;
}

void entity_set_vel(EntityId id, EntityVec vel) {
    if(comps.physics_body.count(id) > 0) {
        auto const& body = comps.physics_body.at(id);
        auto const& bt_quat = body->getOrientation();
        glm::quat quat = {bt_quat.x(), bt_quat.y(), bt_quat.z(), bt_quat.w()};
        vel = glm::rotate(quat, vel);
        vel *= ENTITY_L_VEL;
        body->setLinearVelocity({vel.x, vel.y, vel.z});
    } else {
        LUX_LOG_WARN("entity #%u tried to move despite not having a physics"
                     " body component", id);
    }
}

void entity_rotate_yaw(EntityId id, F32 yaw) {
    if(comps.physics_body.count(id) > 0) {
        auto const& body = comps.physics_body.at(id);
        btTransform tr = body->getCenterOfMassTransform();
        btQuaternion quat;
        quat.setEuler(0.f, -yaw * (tau / 2.f) + tau / 4.f, 0.f);
        tr.setRotation(quat);

        body->setCenterOfMassTransform(tr);
    } else {
        LUX_LOG_WARN("entity #%u tried to rotate despite not having a physics"
                     " body component", id);
    }
}

void entity_rotate_yaw_pitch(EntityId id, Vec2F yaw_pitch) {
    if(comps.physics_body.count(id) > 0) {
        auto const& body = comps.physics_body.at(id);
        btTransform tr = body->getCenterOfMassTransform();
        btQuaternion quat;
        quat.setEuler(yaw_pitch.y * (tau / 2.f),
                      -yaw_pitch.x * (tau / 2.f) + tau / 4.f, 0.f);
        tr.setRotation(quat);

        body->setCenterOfMassTransform(tr);
    } else {
        LUX_LOG_WARN("entity #%u tried to rotate despite not having a physics"
                     " body component", id);
    }
}

void entities_tick() {
    for(auto pair : entities) {
        EntityId id = pair.k;
        if(comps.physics_body.count(id) > 0) {
            auto const& body = comps.physics_body.at(id);
            btVector3 bt_min, bt_max;
            body->getAabb(bt_min, bt_max);
            Vec3F min(bt_min.x(), bt_min.y(), bt_min.z());
            Vec3F max(bt_max.x(), bt_max.y(), bt_max.z());
            guarantee_physics_mesh_for_aabb(floor(min), ceil(max));
        }
    }
    entities.free_slots();
}

void entity_erase(EntityId entity) {
    LUX_ASSERT(entities.contains(entity));
    LUX_LOG("removing entity %u", entity);
    entities.erase(entity);
    if(comps.name.count(entity)         > 0) comps.name.erase(entity);
    if(comps.physics_body.count(entity) > 0) comps.physics_body.erase(entity);
    if(comps.model.count(entity)        > 0) comps.model.erase(entity);
}

void get_net_entity_comps(NetSsTick::EntityComps* net_comps) {
    //@TODO figure out a way to reduce the verbosity of this function
    for(auto const& body : comps.physics_body) {
        auto pos = body.second->getCenterOfMassPosition();
        net_comps->pos[body.first] = Vec3F(pos.x(), pos.y(), pos.z());
    }
    for(auto const& name : comps.name) {
        net_comps->name[name.first] = (Str)name.second;
    }
    for(auto const& model : comps.model) {
        net_comps->model[model.first] =
            {model.second.id};
    }
}
