#include <physics.hpp>

static btDbvtBroadphase                    broadphase;
static btDefaultCollisionConfiguration     collision_conf;
static btCollisionDispatcher               dispatcher(&collision_conf);
static btSequentialImpulseConstraintSolver solver;
static btDiscreteDynamicsWorld             world(&dispatcher, &broadphase,
                                                 &solver, &collision_conf);

static List<btDefaultMotionState> motion_states;
static List<btRigidBody> bodies;

static btCapsuleShapeZ body_shape(1.2, 3.6);

void physics_init() {
    world.setGravity(btVector3(0, 0, 0));
    //@TODO does it has to be static? (takes reference)
    static btVector3 inertia = {0, 0, 0};
    //@TODO is this needed?
    body_shape.calculateLocalInertia(1, inertia);
}

btRigidBody* physics_create_body(EntityVec const& pos) {
    motion_states.emplace_back(btTransform({0, 0, 0, 1}, {pos.x, pos.y, pos.z}));
    btRigidBody::btRigidBodyConstructionInfo ci(1, &motion_states.back(),
        &body_shape, btVector3(0, 0, 0));
    ci.m_friction = 1.0;
    bodies.emplace_back(ci);
    btRigidBody &body = bodies.back();
    body.forceActivationState(DISABLE_DEACTIVATION);
    world.addRigidBody(&body);
    return &body;
}

btRigidBody* physics_create_mesh(MapPos const& pos, btCollisionShape* shape) {
    motion_states.emplace_back(btTransform({0, 0, 0, 1}, {pos.x, pos.y, pos.z}));
    btRigidBody::btRigidBodyConstructionInfo ci(0, &motion_states.back(),
        shape, btVector3(0, 0, 0));
    bodies.emplace_back(ci);
    btRigidBody& body = bodies.back();
    world.addRigidBody(&body);
    return &body;
}

void physics_remove_body(btRigidBody* body) {
    world.removeRigidBody(body);
}

void physics_tick(F32 time) {
    world.stepSimulation(time, 1, time);
}
