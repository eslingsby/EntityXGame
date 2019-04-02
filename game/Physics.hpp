#pragma once

#include <entityx\System.h>

#include "Collider.hpp"
#include <btBulletDynamicsCommon.h>

class Physics : public entityx::System<Physics>, public entityx::Receiver<Physics> {
	btDefaultCollisionConfiguration* _collisionConfiguration = nullptr;
	btCollisionDispatcher* _dispatcher = nullptr;
	btBroadphaseInterface* _overlappingPairCache = nullptr;
	btSequentialImpulseConstraintSolver* _solver = nullptr;
	btDiscreteDynamicsWorld* _dynamicsWorld = nullptr;

public:
	Physics();
	~Physics();

	void configure(entityx::EventManager &events) final;
	void update(entityx::EntityManager &entities, entityx::EventManager &events, double dt) final;

	void receive(const entityx::ComponentAddedEvent<Collider>& colliderAddedEvent);
	void receive(const entityx::ComponentRemovedEvent<Collider>& colliderRemovedEvent);
};