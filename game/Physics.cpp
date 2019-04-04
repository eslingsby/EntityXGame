#include "Physics.hpp"

#include <btBulletCollisionCommon.h>
#include "Transform.hpp"
#include "PhysicsEvents.hpp"

Physics::Physics(const ConstructorInfo& constructorInfo) : _constructorInfo(constructorInfo) {
	_collisionConfiguration = new btDefaultCollisionConfiguration();
	_dispatcher = new btCollisionDispatcher(_collisionConfiguration);
	_overlappingPairCache = new btDbvtBroadphase();
	_solver = new btSequentialImpulseConstraintSolver;
	_dynamicsWorld = new btDiscreteDynamicsWorld(_dispatcher, _overlappingPairCache, _solver, _collisionConfiguration);

	_dynamicsWorld->setGravity(toBt(_constructorInfo.defaultGravity));
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
	for (auto entity : entities.entities_with_components<Transform, Collider>()) {
		//glm::vec3 globalPosition;
		//glm::quat globalRotation;

		//glm::mat4 globalMatrix = entity.component<Transform>()->globalMatrix();

		const auto transform = entity.component<Transform>();

		//entity.component<Transform>()->globalDecomposed(&globalPosition, &globalRotation, nullptr);
		
		//auto collider = entity.component<Collider>();
		btTransform colliderTransform;


		//colliderTransform.setFromOpenGLMatrix((btScalar*)&globalMatrix[0]);

		colliderTransform.setOrigin(toBt(transform->position));
		colliderTransform.setRotation(toBt(transform->rotation));

		entity.component<Collider>()->rigidBody->setWorldTransform(colliderTransform);
	}

	for (uint32_t i = 0; i < _constructorInfo.stepsPerUpdate; i++) {
		_dynamicsWorld->stepSimulation((btScalar)(dt) / _constructorInfo.stepsPerUpdate, _constructorInfo.maxSubSteps);
		events.emit<PhysicsUpdateEvent>(PhysicsUpdateEvent{ entities, dt, _constructorInfo.stepsPerUpdate, i });
	}
	
	//for (uint32_t i = 0; i < _dispatcher->getNumManifolds(); i++) {
	//	const auto manifold = _dispatcher->getManifoldByIndexInternal(i);
	//
	//	if (!manifold->getNumContacts())
	//		continue;
	//
	//	const Collider* firstCollider = (Collider*)manifold->getBody0()->getUserPointer();
	//	const Collider* secondCollider = (Collider*)manifold->getBody1()->getUserPointer();
	//
	//	for (uint32_t x = 0; x < manifold->getNumContacts(); x++) {
	//		const auto point = manifold->getContactPoint(x);
	//
	//		events.emit<ContactEvent>(ContactEvent{ firstCollider->self, secondCollider->self, fromBt(point.getPositionWorldOnB()), fromBt(point.m_normalWorldOnB) });
	//	}
	//}
}

void Physics::setGravity(const glm::vec3 & gravity){
	_dynamicsWorld->setGravity(toBt(gravity));
}

void Physics::receive(const entityx::ComponentAddedEvent<Collider>& colliderAddedEvent) {
	auto collider = colliderAddedEvent.component;

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
	
	if (collider->rigidInfo.mass != 0.f)
		collider->collisionShape->calculateLocalInertia(collider->rigidInfo.mass, localInertia);

	btRigidBody::btRigidBodyConstructionInfo rbInfo(collider->rigidInfo.mass, (btMotionState*)collider.get(), collider->collisionShape, localInertia);
	collider->rigidBody = new btRigidBody(rbInfo);

	collider->rigidBody->setGravity(toBt(collider->rigidInfo.gravity));
	collider->rigidBody->setUserPointer(collider.get());

	if (collider->rigidInfo.kinematic)
		collider->rigidBody->setFlags(collider->rigidBody->getFlags() | btCollisionObject::CollisionFlags::CF_KINEMATIC_OBJECT);
	
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
