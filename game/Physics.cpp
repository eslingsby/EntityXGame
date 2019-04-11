#include "Physics.hpp"

#include <btBulletCollisionCommon.h>
#include "Transform.hpp"
#include "PhysicsEvents.hpp"

std::vector<CollidingEvent> collidingEvents;
std::vector<ContactEvent> contactEvents;

void contactStartedCallback(btPersistentManifold* const& manifold) {
	Collider* firstCollider = (Collider*)manifold->getBody0()->getUserPointer();
	Collider* secondCollider = (Collider*)manifold->getBody1()->getUserPointer();

	if (firstCollider->bodyInfo.callbacks)
		collidingEvents.push_back(CollidingEvent{ true, ContactEvent{ firstCollider->self, secondCollider->self } });
}

void contactEndedCallback(btPersistentManifold* const& manifold) {
	Collider* firstCollider = (Collider*)manifold->getBody0()->getUserPointer();
	Collider* secondCollider = (Collider*)manifold->getBody1()->getUserPointer();

	if (firstCollider->bodyInfo.callbacks)
		collidingEvents.push_back(CollidingEvent{ false, ContactEvent{ firstCollider->self, secondCollider->self} });
}

bool contactAddedCallback(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1) {
	Collider* firstCollider = (Collider*)colObj0Wrap->getCollisionObject()->getUserPointer();
	Collider* secondCollider = (Collider*)colObj1Wrap->getCollisionObject()->getUserPointer();

	if (firstCollider->bodyInfo.callbacks)
		contactEvents.push_back(ContactEvent{ firstCollider->self, secondCollider->self });

	return false;
}

Physics::Physics(const ConstructorInfo& constructorInfo) : _constructorInfo(constructorInfo) {
	_collisionConfiguration = new btDefaultCollisionConfiguration();
	_dispatcher = new btCollisionDispatcher(_collisionConfiguration);
	_overlappingPairCache = new btDbvtBroadphase();
	_solver = new btSequentialImpulseConstraintSolver;
	_dynamicsWorld = new btDiscreteDynamicsWorld(_dispatcher, _overlappingPairCache, _solver, _collisionConfiguration);

	setGravity(_constructorInfo.defaultGravity);

	gContactAddedCallback = contactAddedCallback;
	gContactStartedCallback = contactStartedCallback;
	gContactEndedCallback = contactEndedCallback;
}

Physics::~Physics(){
	if (_dynamicsWorld)
		delete _dynamicsWorld;
	if (_solver)
		delete _solver;
	if (_overlappingPairCache)
		delete _overlappingPairCache;
	if (_dispatcher)
		delete _dispatcher;
	if (_collisionConfiguration)
		delete _collisionConfiguration;
}

void Physics::configure(entityx::EventManager & events){
	events.subscribe<entityx::ComponentAddedEvent<Collider>>(*this);
	events.subscribe<entityx::ComponentRemovedEvent<Collider>>(*this);
}

void Physics::update(entityx::EntityManager & entities, entityx::EventManager & events, double dt){
	// Copy over transform data to collider
	for (auto entity : entities.entities_with_components<Transform, Collider>()) {
		auto transform = entity.component<Transform>();
		auto collider = entity.component<Collider>();
		
		glm::vec3 globalPosition;
		glm::quat globalRotation;

		transform->globalDecomposed(&globalPosition, &globalRotation);
		
		btTransform newTransform;

		newTransform.setOrigin(toBt(globalPosition));
		newTransform.setRotation(toBt(globalRotation));

		collider->rigidBody.setWorldTransform(newTransform);
	}

	// Step the simulation, emitting events each step
	for (uint32_t i = 0; i < _constructorInfo.stepsPerUpdate; i++) {
		_dynamicsWorld->stepSimulation((btScalar)(dt) / _constructorInfo.stepsPerUpdate, _constructorInfo.maxSubSteps);

		for (const CollidingEvent& collidingEvent : collidingEvents)
			events.emit<CollidingEvent>(collidingEvent);

		collidingEvents.clear();

		for (const ContactEvent& contactEvent : contactEvents)
			events.emit<ContactEvent>(contactEvent);

		contactEvents.clear();

		events.emit<PhysicsUpdateEvent>(PhysicsUpdateEvent{ entities, dt, _constructorInfo.stepsPerUpdate, i });
	}
}

void Physics::setGravity(const glm::vec3 & gravity){
	_dynamicsWorld->setGravity(toBt(gravity));
}

void Physics::receive(const entityx::ComponentAddedEvent<Collider>& colliderAddedEvent) {
	auto collider = colliderAddedEvent.component;

	collider->self = colliderAddedEvent.entity;

	btTransform shapeTransform;
	shapeTransform.setIdentity();

	const Collider::ShapeInfo& shapeInfo = collider->shapeInfo;

	switch (collider->shapeInfo.type) {
	case Collider::Sphere:
		//collider->collisionShape = new btSphereShape(shapeInfo.a * .5f);
		collider->shapeVariant = btSphereShape(shapeInfo.a * .5f);
		break;
	case Collider::Box:
		//collider->collisionShape = new btBoxShape(btVector3(shapeInfo.a * .5f, shapeInfo.b * .5f, shapeInfo.c * .5f));
		collider->shapeVariant = btBoxShape(btVector3(shapeInfo.a * .5f, shapeInfo.b * .5f, shapeInfo.c * .5f));
		break;
	case Collider::Plane:
		//collider->collisionShape = new btStaticPlaneShape(btVector3(0, 0, 1), 1);
		collider->shapeVariant = btStaticPlaneShape(btVector3(0, 0, 1), 1);
		break;
	case Collider::Capsule:
		//collider->collisionShape = new btCapsuleShapeZ(shapeInfo.a * .5f, shapeInfo.b);
		collider->shapeVariant = btCapsuleShapeZ(shapeInfo.a * .5f, shapeInfo.b);
		break;
	case Collider::Cylinder:
		//collider->collisionShape = new btCylinderShapeZ(btVector3(shapeInfo.a * .5f, shapeInfo.a * .5f, shapeInfo.b * .5f));
		collider->shapeVariant = btCylinderShapeZ(btVector3(shapeInfo.a * .5f, shapeInfo.a * .5f, shapeInfo.b * .5f));
		break;
	case Collider::Cone:
		//collider->collisionShape = new btConeShapeZ(shapeInfo.a * .5f, shapeInfo.b);
		collider->shapeVariant = btConeShapeZ(shapeInfo.a * .5f, shapeInfo.b);
		break;
	}
	
	btVector3 localInertia(0.f, 0.f, 0.f);
	float mass = collider->bodyInfo.mass;

	//if (collider->bodyInfo.type == Collider::Static || collider->bodyInfo.type == Collider::Trigger)
	//	mass = 0.f;

	btCollisionShape* shape;// = collider->collisionShape;

	std::visit([&](btCollisionShape& visitShape) {
		shape = &visitShape;
	}, collider->shapeVariant);

	if (colliderAddedEvent.entity.has_component<Transform>() && collider->shapeInfo.type != Collider::Plane) {
		auto transform = colliderAddedEvent.entity.component<const Transform>();
		shape->setLocalScaling(toBt(transform->scale));
	}

	if (mass != 0.f)
		shape->calculateLocalInertia(collider->bodyInfo.mass, localInertia);

	collider->rigidBody = btRigidBody(mass, (btMotionState*)collider.get(), shape, localInertia);
	collider->rigidBody.setUserPointer(collider.get());

	switch (collider->bodyInfo.type) {
	case Collider::Static:
		collider->rigidBody.setCollisionFlags(btCollisionObject::CollisionFlags::CF_STATIC_OBJECT);
		break;
	case Collider::StaticTrigger:
		collider->rigidBody.setCollisionFlags(btCollisionObject::CollisionFlags::CF_STATIC_OBJECT | btCollisionObject::CollisionFlags::CF_NO_CONTACT_RESPONSE);
		break;
	case Collider::Trigger:
		collider->rigidBody.setCollisionFlags(btCollisionObject::CollisionFlags::CF_NO_CONTACT_RESPONSE);
		break;
	}

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

	_dynamicsWorld->addRigidBody(&collider->rigidBody);
}

void Physics::receive(const entityx::ComponentRemovedEvent<Collider>& colliderRemovedEvent){
	auto collider = colliderRemovedEvent.component;

	_dynamicsWorld->removeRigidBody(&collider->rigidBody);
	
	//if (collider->collisionShape)
	//	delete collider->collisionShape;
}
