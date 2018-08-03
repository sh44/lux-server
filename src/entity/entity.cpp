#include <cmath>
#include <world.hpp>
#include "entity.hpp"

Entity::Entity(World const &world, entity::Type const &type, btRigidBody *body) :
    world(world),
    type(type),
    body(body),
    deletion_mark(false)
{
    body->forceActivationState(DISABLE_DEACTIVATION);
}

entity::Pos Entity::get_pos() const
{
    auto pos = body->getCenterOfMassPosition();
    return entity::Pos(pos.x(), pos.y(), pos.z());
}

void Entity::update()
{
    world.guarantee_chunk(chunk::to_pos(get_pos()));
}

void Entity::move(entity::Vec const &by)
{
    body->setLinearVelocity({by.x * 20.f, by.y * 20.f, by.z});
}
