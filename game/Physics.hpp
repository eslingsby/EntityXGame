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
	struct ConstructorInfo {
		glm::vec3 defaultGravity = { 0.f, 0.f, -1000.f };
		uint32_t stepsPerUpdate = 1;
		uint32_t maxSubSteps = 1;
	};

private:
	const ConstructorInfo _constructorInfo;

public:
	Physics(const ConstructorInfo& constructorInfo = ConstructorInfo());
	~Physics();

	void configure(entityx::EventManager &events) final;
	void update(entityx::EntityManager &entities, entityx::EventManager &events, double dt) final;

	void setGravity(const glm::vec3& gravity);

	void receive(const entityx::ComponentAddedEvent<Collider>& colliderAddedEvent);
	void receive(const entityx::ComponentRemovedEvent<Collider>& colliderRemovedEvent);
};