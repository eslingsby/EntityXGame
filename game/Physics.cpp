#include "Physics.hpp"

#include <btBulletCollisionCommon.h>
#include "Transform.hpp"
#include "PhysicsEvents.hpp"

/*
typedef bool (*ContactDestroyedCallback)(void* userPersistentData);
typedef bool (*ContactProcessedCallback)(btManifoldPoint& cp, void* body0, void* body1);
typedef void (*ContactStartedCallback)(btPersistentManifold* const& manifold);
typedef void (*ContactEndedCallback)(btPersistentManifold* const& manifold);

typedef bool (*ContactAddedCallback)(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1);
extern ContactAddedCallback gContactAddedCallback;

///This is to allow MaterialCombiner/Custom Friction/Restitution values
ContactAddedCallback gContactAddedCallback = 0;
*/

std::vector<CollidingEvent> collidingEvents;
std::vector<ContactEvent> contactEvents;

void contactStartedCallback(btPersistentManifold* const& manifold) {
	Collider* collider0 = (Collider*)manifold->getBody0()->getUserPointer();
	Collider* collider1 = (Collider*)manifold->getBody1()->getUserPointer();

	collidingEvents.push_back(CollidingEvent{ true, ContactEvent{ collider0->self, collider1->self } });
}

void contactEndedCallback(btPersistentManifold* const& manifold) {
	Collider* collider0 = (Collider*)manifold->getBody0()->getUserPointer();
	Collider* collider1 = (Collider*)manifold->getBody1()->getUserPointer();

	collidingEvents.push_back(CollidingEvent{ false, ContactEvent{collider0->self, collider1->self} });
}

bool contactAddedCallback(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1) {
	Collider* collider0 = (Collider*)colObj0Wrap->getCollisionObject()->getUserPointer();
	Collider* collider1 = (Collider*)colObj1Wrap->getCollisionObject()->getUserPointer();

	contactEvents.push_back(ContactEvent{ collider0->self, collider1->self });

	return false;
}

Physics::Physics(const ConstructorInfo& constructorInfo) : _constructorInfo(constructorInfo) {
	_collisionConfiguration = new btDefaultCollisionConfiguration();
	_dispatcher = new btCollisionDispatcher(_collisionConfiguration);
	_overlappingPairCache = new btDbvtBroadphase();
	_solver = new btSequentialImpulseConstraintSolver;
	_dynamicsWorld = new btDiscreteDynamicsWorld(_dispatcher, _overlappingPairCache, _solver, _collisionConfiguration);

	_dynamicsWorld->setGravity(toBt(_constructorInfo.defaultGravity));

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

	//_eventsPtr = &events;
}

void Physics::update(entityx::EntityManager & entities, entityx::EventManager & events, double dt){
	for (auto entity : entities.entities_with_components<Transform, Collider>()) {
		//glm::vec3 globalPosition;
		//glm::quat globalRotation;
		//
		//entity.component<Transform>()->globalDecomposed(&globalPosition, &globalRotation);
		//
		////glm::mat4 globalMatrix = entity.component<Transform>()->globalMatrix();
		//
		//btTransform colliderTransform;
		//
		////olliderTransform.setFromOpenGLMatrix((btScalar*)&globalMatrix[0]);
		//
		//colliderTransform.setOrigin(toBt(globalPosition));
		//colliderTransform.setRotation(toBt(globalRotation));
		//
		//auto collider = entity.component<Collider>();
		//
		//collider->rigidBody->setWorldTransform(colliderTransform);
		//
		////collider->rigidBody->setSca
	

		auto transform = entity.component<Transform>();
		auto collider = entity.component<Collider>();
		
		glm::vec3 globalPosition;
		glm::quat globalRotation;

		transform->globalDecomposed(&globalPosition, &globalRotation);
		
		btTransform newTransform;

		newTransform.setOrigin(toBt(globalPosition));
		newTransform.setRotation(toBt(globalRotation));

		collider->rigidBody->setWorldTransform(newTransform);
	
	}

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

	const Collider::ShapeInfo& shapeInfo = collider->shapeInfo;

	switch (collider->shapeInfo.type) {
	case Collider::Sphere:
		collider->collisionShape = new btSphereShape(shapeInfo.a);
		break;
	case Collider::Box:
		collider->collisionShape = new btBoxShape(btVector3(shapeInfo.a, shapeInfo.b, shapeInfo.c));
		break;
	case Collider::Plane:
		collider->collisionShape = new btStaticPlaneShape(btVector3(shapeInfo.a, shapeInfo.b, shapeInfo.c), shapeInfo.d);
		break;
	case Collider::Capsule:
		collider->collisionShape = new btCapsuleShapeZ(shapeInfo.a, shapeInfo.b);
		break;
	case Collider::Cylinder:
		collider->collisionShape = new btCylinderShape(btVector3(shapeInfo.a, shapeInfo.b, shapeInfo.c));
		break;
	}

	btVector3 localInertia(0.f, 0.f, 0.f);

	float mass = collider->bodyInfo.mass;
	
	if (collider->bodyInfo.type == Collider::Static || collider->bodyInfo.type == Collider::Trigger)
		mass = 0.f;

	if (mass != 0.f)
		collider->collisionShape->calculateLocalInertia(mass, localInertia);

	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, (btMotionState*)collider.get(), collider->collisionShape, localInertia);
	collider->rigidBody = new btRigidBody(rbInfo);

	//collider->rigidBody->setGravity(toBt(collider->bodyInfo.gravity));
	collider->rigidBody->setUserPointer(collider.get());

	switch (collider->bodyInfo.type) {
	//case Collider::Kinematic:
	//	collider->rigidBody->setCollisionFlags(btCollisionObject::CollisionFlags::CF_KINEMATIC_OBJECT);
	//	break;

	case Collider::Static:
		collider->rigidBody->setCollisionFlags(btCollisionObject::CollisionFlags::CF_STATIC_OBJECT);
		break;

	case Collider::StaticTrigger:
		collider->rigidBody->setCollisionFlags(btCollisionObject::CollisionFlags::CF_STATIC_OBJECT | btCollisionObject::CollisionFlags::CF_NO_CONTACT_RESPONSE);
		break;

	case Collider::Trigger:
		collider->rigidBody->setCollisionFlags(btCollisionObject::CollisionFlags::CF_NO_CONTACT_RESPONSE);
		break;
	}

	collider->rigidBody->setAngularFactor(toBt(collider->bodyInfo.defaultAngularFactor));
	collider->rigidBody->setAngularFactor(toBt(collider->bodyInfo.defaultAngularFactor));
	collider->rigidBody->setAngularVelocity(toBt(collider->bodyInfo.startingAngularVelocity));
	collider->rigidBody->setAngularVelocity(toBt(collider->bodyInfo.startingLinearVelocity));

	if (collider->bodyInfo.alwaysActive)// || collider->bodyInfo.type == Collider::Trigger || collider->bodyInfo.type == Collider::StaticTrigger)
		collider->setAlwaysActive(true);

	if (collider->bodyInfo.callbacks)
		collider->rigidBody->setCollisionFlags(collider->rigidBody->getCollisionFlags() | btCollisionObject::CollisionFlags::CF_CUSTOM_MATERIAL_CALLBACK);
	
	_dynamicsWorld->addRigidBody(collider->rigidBody);
}

void Physics::receive(const entityx::ComponentRemovedEvent<Collider>& colliderRemovedEvent){
	auto collider = colliderRemovedEvent.component;

	_dynamicsWorld->removeRigidBody(collider->rigidBody);

	if (collider->rigidBody)
		delete collider->rigidBody;

	if (collider->collisionShape)
		delete collider->collisionShape;
}
