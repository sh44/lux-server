#include <world.hpp>
#include "entity.hpp"

Entity::Entity(World const &world, entity::Type const &type, btRigidBody *body) :
    world(world),
    type(type),
    body(body)
{

}

entity::Pos Entity::get_pos() const
{
    auto pos = body->getCenterOfMassPosition();
    return entity::Pos(pos.x(), pos.y(), pos.z());
}

void Entity::update()
{
    world[get_pos()]; //TODO nasty trick to load the chunk
}

void Entity::move(entity::Vec const &by)
{
    body->setLinearVelocity({by.x * 20.f, by.y * 20.f, by.z + 0.1f});
}
