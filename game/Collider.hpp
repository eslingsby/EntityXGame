#pragma once

#include <LinearMath\btMotionState.h>
#include <entityx\Entity.h>
#include <glm\gtx\matrix_decompose.hpp>
#include <btBulletDynamicsCommon.h>

#include "Transform.hpp"

inline glm::quat fromBt(const btQuaternion& from) {
	return glm::quat(from.w(), from.x(), from.y(), from.z());
}

inline glm::vec3 fromBt(const btVector3& from) {
	return glm::vec3(from.x(), from.y(), from.z());
}

inline btQuaternion toBt(const glm::quat& from) {
	return btQuaternion(from.x, from.y, from.z, from.w);
}

inline btVector3 toBt(const glm::vec3& from) {
	return btVector3(from.x, from.y, from.z);
}

struct Collider : public btMotionState {
	enum ShapeType {
		Sphere, // a = radius
		Box, // a = width, b = height, c = depth
		Cylinder, // a = radius, b = height
		Capsule, // a = radius, b = height
		Plane
	};

	enum BodyType {
		Dynamic,
		Static,
		//Kinematic,
		Trigger,
		StaticTrigger
	};

	struct ShapeInfo {
		ShapeType type = Sphere;

		float a = 0.f;
		float b = 0.f;
		float c = 0.f;
		float d = 0.f;
	};

	struct BodyInfo {
		BodyType type = Dynamic;
		float mass = 0.f;

		bool alwaysActive = false;
		bool callbacks = false;

		glm::vec3 defaultLinearFactor = { 1, 1, 1 };
		glm::vec3 defaultAngularFactor = { 1, 1, 1 };

		glm::vec3 startingLinearVelocity;
		glm::vec3 startingAngularVelocity;

		glm::vec3 centerOfMass;

		//float friction = 0.5f;
		//float restitution = 0.f;
		//float linearThreshold = 1.f;
		//float angularThreshold = 0.8f;
	};

	entityx::Entity self;

	const ShapeInfo shapeInfo;
	const BodyInfo bodyInfo;

	btCollisionShape* collisionShape = nullptr;
	btRigidBody* rigidBody = nullptr;

	Collider(ShapeInfo shapeInfo, BodyInfo bodyInfo = BodyInfo());

	void getWorldTransform(btTransform& worldTransform) const final;
	void setWorldTransform(const btTransform& worldTransform) final;

	void setActive(bool active);
	void setAlwaysActive(bool alwaysActive);
	void setLinearVelocity(const glm::vec3& velocity);
	void setAngularVelocity(const glm::vec3& velocity);
	void setLinearFactor(const glm::vec3& factor);
	void setAngularFactor(const glm::vec3& factor);
};