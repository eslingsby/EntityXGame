#pragma once

#include "component\Collider.hpp"

#include "other\BulletDebug.hpp"

#include <entityx\System.h>

#include <btBulletDynamicsCommon.h>

class Physics : public entityx::System<Physics>, public entityx::Receiver<Physics> {
	btDefaultCollisionConfiguration _collisionConfiguration;
	btCollisionDispatcher _dispatcher;
	btDbvtBroadphase _overlappingPairCache;
	btSequentialImpulseConstraintSolver _solver;
	btDiscreteDynamicsWorld _dynamicsWorld;

	BulletDebug _debugger;

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

	void configure(entityx::EventManager &events) final;
	void update(entityx::EntityManager &entities, entityx::EventManager &events, double dt) final;

	void receive(const entityx::ComponentAddedEvent<Collider>& colliderAddedEvent);
	void receive(const entityx::ComponentRemovedEvent<Collider>& colliderRemovedEvent);

	void setGravity(const glm::vec3& gravity);

	void rayTest(const glm::vec3& from, const glm::vec3& to, std::vector<entityx::Entity>& hits);

	const BulletDebug& bulletDebug() const;
};