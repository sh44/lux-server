#include <glm/glm.hpp>
//
#include <world.hpp>
#include "entity.hpp"

Entity::Entity(World &world, EntityType const &type, btRigidBody *body) :
    world(world),
    type(type),
    deletion_mark(false),
    body(body)
{
    body->forceActivationState(DISABLE_DEACTIVATION);
}

EntityPos Entity::get_pos() const
{
    auto pos = body->getCenterOfMassPosition();
    return EntityPos(pos.x(), pos.y(), pos.z());
}

void Entity::tick()
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
                world.guarantee_mesh(iter);
            }
        }
    }
}

void Entity::move(EntityVec const &by)
{
    body->setLinearVelocity({by.x * 30.f, by.y * 30.f,
                             body->getLinearVelocity().z()});
}

void Entity::jump()
{
    body->applyCentralImpulse({0.f, 0.f, 0.3f});
}
