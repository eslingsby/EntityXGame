#include <btBulletCollisionCommon.h>
#include <BulletCollision\NarrowPhaseCollision\btRaycastCallback.h>

#include "component\Transform.hpp"

#include "system\Physics.hpp"
#include "system\PhysicsEvents.hpp"

entityx::EventManager* eventsPtr;

void contactStartedCallback(btPersistentManifold* const& manifold) {
	Collider* firstCollider = (Collider*)manifold->getBody0()->getUserPointer();
	Collider* secondCollider = (Collider*)manifold->getBody1()->getUserPointer();

	//std::cout << "Contact started with " << firstCollider->self.id() << ", " << secondCollider->self.id() << '\n';

	eventsPtr->emit<CollidingEvent>(CollidingEvent{ true, ContactEvent{ firstCollider->self, secondCollider->self } });
}

void contactEndedCallback(btPersistentManifold* const& manifold) {
	Collider* firstCollider = (Collider*)manifold->getBody0()->getUserPointer();
	Collider* secondCollider = (Collider*)manifold->getBody1()->getUserPointer();

	//std::cout << "Contact ended with " << firstCollider->self.id() << ", " << secondCollider->self.id() << '\n';

	eventsPtr->emit<CollidingEvent>(CollidingEvent{ false, ContactEvent{ firstCollider->self, secondCollider->self } });
}

bool contactAddedCallback(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1) {
	Collider* firstCollider = (Collider*)colObj0Wrap->getCollisionObject()->getUserPointer();
	Collider* secondCollider = (Collider*)colObj1Wrap->getCollisionObject()->getUserPointer();

	eventsPtr->emit<ContactEvent>(ContactEvent{ firstCollider->self, secondCollider->self });

	return false;
}

Physics::Physics(const ConstructorInfo& constructorInfo) :
		_constructorInfo(constructorInfo), 
		_dispatcher(&_collisionConfiguration), 
		_dynamicsWorld(&_dispatcher, &_overlappingPairCache, &_solver, &_collisionConfiguration) {

	_dynamicsWorld.setDebugDrawer(&_debugger);

	setGravity(_constructorInfo.defaultGravity);

	gContactAddedCallback = contactAddedCallback;
	gContactStartedCallback = contactStartedCallback;
	gContactEndedCallback = contactEndedCallback;
}

void Physics::configure(entityx::EventManager & events){
	events.subscribe<entityx::ComponentAddedEvent<Collider>>(*this);
	events.subscribe<entityx::ComponentRemovedEvent<Collider>>(*this);

	eventsPtr = &events;
}

void Physics::update(entityx::EntityManager & entities, entityx::EventManager & events, double dt){
	// Copy over transform data to collider
	for (auto entity : entities.entities_with_components<Transform, Collider>()) {
		auto transform = entity.component<Transform>();
		auto collider = entity.component<Collider>();
		
		// If position / rotation changed
		glm::vec3 globalPosition;
		glm::quat globalRotation;
		glm::vec3 globalScale;

		transform->globalDecomposed(&globalPosition, &globalRotation, &globalScale);
		
		btTransform newTransform;

		newTransform.setOrigin(toBt(globalPosition));
		newTransform.setRotation(toBt(globalRotation));

		collider->rigidBody.setWorldTransform(newTransform);

		// If scale, mass, or center of mass changed
		btCollisionShape* collisionShape;

		std::visit([&](btCollisionShape& shape) {
			collisionShape = &shape;
		}, collider->shapeVariant);

		if (fromBt(collisionShape->getLocalScaling()) != globalScale) {
			_dynamicsWorld.removeRigidBody(&collider->rigidBody);

			collisionShape->setLocalScaling(toBt(globalScale));

			btVector3 inertia;
			collisionShape->calculateLocalInertia(collider->bodyInfo.mass, inertia);

			collider->rigidBody.setMassProps(collider->bodyInfo.mass, inertia);

			_dynamicsWorld.addRigidBody(&collider->rigidBody);
		}
	}

	// Step the simulation, pumping collision events each step
	for (uint32_t i = 0; i < _constructorInfo.stepsPerUpdate; i++) {
		_dynamicsWorld.stepSimulation((btScalar)(dt) / _constructorInfo.stepsPerUpdate, _constructorInfo.maxSubSteps);

		events.emit<PhysicsUpdateEvent>(PhysicsUpdateEvent{ entities, dt, _constructorInfo.stepsPerUpdate, i });
	}

	// Draw bullet world
	_debugger.clearLines();
	_dynamicsWorld.debugDrawWorld();
}

void Physics::receive(const entityx::ComponentAddedEvent<Collider>& colliderAddedEvent) {
	auto collider = colliderAddedEvent.component;

	collider->self = colliderAddedEvent.entity;

	btTransform shapeTransform;
	shapeTransform.setIdentity();

	const Collider::ShapeInfo& shapeInfo = collider->shapeInfo;

	switch (collider->shapeInfo.type) {
	case Collider::Sphere:
		collider->shapeVariant = btSphereShape(shapeInfo.a * .5f);
		break;
	case Collider::Box:
		collider->shapeVariant = btBoxShape(btVector3(shapeInfo.a * .5f, shapeInfo.b * .5f, shapeInfo.c * .5f));
		break;
	case Collider::Plane:
		collider->shapeVariant = btStaticPlaneShape(btVector3(0, 0, 1), 1);
		break;
	case Collider::Capsule:
		collider->shapeVariant = btCapsuleShapeZ(shapeInfo.a * .5f, shapeInfo.b);
		break;
	case Collider::Cylinder:
		collider->shapeVariant = btCylinderShapeZ(btVector3(shapeInfo.a * .5f, shapeInfo.a * .5f, shapeInfo.b * .5f));
		break;
	case Collider::Cone:
		collider->shapeVariant = btConeShapeZ(shapeInfo.a * .5f, shapeInfo.b);
		break;
	}

	btCollisionShape* shape;

	std::visit([&](btCollisionShape& visitShape) {
		shape = &visitShape;
	}, collider->shapeVariant);

	if (colliderAddedEvent.entity.has_component<Transform>() && collider->shapeInfo.type != Collider::Plane) {
		auto transform = colliderAddedEvent.entity.component<const Transform>();

		glm::vec3 globalScale;
		transform->globalDecomposed(nullptr, nullptr, &globalScale);

		shape->setLocalScaling(toBt(globalScale));
	}

	btVector3 localInertia(0.f, 0.f, 0.f);

	if (collider->bodyInfo.mass != 0.f)
		shape->calculateLocalInertia(collider->bodyInfo.mass, localInertia);

	collider->rigidBody = btRigidBody(collider->bodyInfo.mass, (btMotionState*)collider.get(), shape, localInertia);
	collider->rigidBody.setUserPointer(collider.get());

	if (collider->bodyInfo.type == Collider::Trigger)
		collider->rigidBody.setCollisionFlags(btCollisionObject::CollisionFlags::CF_NO_CONTACT_RESPONSE);

	collider->setAngularFactor(collider->bodyInfo.defaultAngularFactor);
	collider->setAngularFactor(collider->bodyInfo.defaultAngularFactor);
	collider->setFriction(collider->bodyInfo.defaultFriction);
	collider->setRestitution(collider->bodyInfo.defaultRestitution);

	collider->setAngularVelocity(collider->bodyInfo.startingAngularVelocity);
	collider->setAngularVelocity(collider->bodyInfo.startingLinearVelocity);

	if (collider->bodyInfo.alwaysActive)
		collider->setAlwaysActive(true);

	if (collider->bodyInfo.callbacks)
		collider->rigidBody.setCollisionFlags(collider->rigidBody.getCollisionFlags() | btCollisionObject::CollisionFlags::CF_CUSTOM_MATERIAL_CALLBACK);

	_dynamicsWorld.addRigidBody(&collider->rigidBody);

	//std::cout << "Collider added " << colliderAddedEvent.entity.id() << '\n';
}

void Physics::receive(const entityx::ComponentRemovedEvent<Collider>& colliderRemovedEvent){
	auto collider = colliderRemovedEvent.component;

	_dynamicsWorld.removeRigidBody(&collider->rigidBody);

	//std::cout << "Collider removed " << colliderRemovedEvent.entity.id() << '\n';
}

void Physics::setGravity(const glm::vec3 & gravity) {
	_dynamicsWorld.setGravity(toBt(gravity));
}

void Physics::rayTest(const glm::vec3& from, const glm::vec3& to, std::vector<entityx::Entity>& hits){
	btVector3 bFrom(toBt(from));
	btVector3 bTo(toBt(to));

	btCollisionWorld::AllHitsRayResultCallback results(bFrom, bTo);

	results.m_flags |= btTriangleRaycastCallback::kF_KeepUnflippedNormal;
	results.m_flags |= btTriangleRaycastCallback::kF_UseSubSimplexConvexCastRaytest;

	_dynamicsWorld.rayTest(bFrom, bTo, results);

	hits.reserve(results.m_collisionObjects.size());

	for (uint32_t i = 0; i < results.m_collisionObjects.size(); i++) {
		Collider* collider = (Collider*)results.m_collisionObjects[i]->getUserPointer();
		hits.push_back(collider->self);
	}
}

const BulletDebug & Physics::bulletDebug() const{
	return _debugger;
}
