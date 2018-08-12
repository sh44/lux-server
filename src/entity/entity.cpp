#include <glm/glm.hpp>
//
#include <world.hpp>
#include "entity.hpp"

Entity::Entity(World const &world, entity::Type const &type, btRigidBody *body) :
    world(world),
    type(type),
    deletion_mark(false),
    body(body)
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
    ChkPos center = to_chk_pos(glm::round(get_pos()));
    ChkPos iter;
    for(iter.z = center.z - 1; iter.z <= center.z + 1; ++iter.z)
    {
        for(iter.y = center.y - 1; iter.y <= center.y + 1; ++iter.y)
        {
            for(iter.x = center.x - 1; iter.x <= center.x + 1; ++iter.x)
            {
                world.guarantee_chunk(iter);
            }
        }
    }
}

void Entity::move(entity::Vec const &by)
{
    body->setLinearVelocity({by.x * 20.f, by.y * 20.f, by.z});
}

void Entity::jump()
{
    body->applyCentralImpulse({0.f, 0.f, 0.5f});
}
