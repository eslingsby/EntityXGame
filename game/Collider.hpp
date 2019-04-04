#pragma once

#include <LinearMath\btMotionState.h>
#include <entityx\Entity.h>
#include <glm\gtx\matrix_decompose.hpp>
#include <btBulletDynamicsCommon.h>

#include "Transform.hpp"

inline glm::quat fromBt(const btQuaternion& from) {
	return glm::quat{
		static_cast<float>(from.w()),
		static_cast<float>(from.x()),
		static_cast<float>(from.y()),
		static_cast<float>(from.z())
	};
}

inline glm::vec3 fromBt(const btVector3& from) {
	return glm::vec3{
		static_cast<float>(from.x()),
		static_cast<float>(from.y()),
		static_cast<float>(from.z())
	};
}

inline float fromBt(const btScalar& from) {
	return static_cast<float>(from);
}

inline btQuaternion toBt(const glm::quat& from) {
	return btQuaternion{
		static_cast<btScalar>(from.x),
		static_cast<btScalar>(from.y),
		static_cast<btScalar>(from.z),
		static_cast<btScalar>(from.w)
	};
}

inline btVector3 toBt(const glm::vec3& from) {
	return btVector3{
		static_cast<btScalar>(from.x),
		static_cast<btScalar>(from.y),
		static_cast<btScalar>(from.z)
	};
}

struct Collider : public btMotionState {
	enum ShapeType {
		Sphere, // a = radius
		Box, // a = width, b = height, c = depth
		Cylinder, // a = radius, b = height
		Capsule, // a = radius, b = height
		Plane
	};

	struct ShapeInfo {
		ShapeType type = Sphere;

		float a = 0.f;
		float b = 0.f;
		float c = 0.f;
		float d = 0.f;
	};

	struct RigidInfo {
		glm::vec3 gravity;
		float mass = 10.f;

		bool kinematic = false;

		glm::vec3 centerOfMass;

		//float friction = 0.5f;
		//float restitution = 0.f;
		//float linearThreshold = 1.f;
		//float angularThreshold = 0.8f;
	};

	entityx::Entity self;

	const ShapeInfo shapeInfo;
	const RigidInfo rigidInfo;

	btCollisionShape* collisionShape = nullptr;
	btRigidBody* rigidBody = nullptr;

	Collider(entityx::Entity self, ShapeInfo shapeInfo, RigidInfo rigidInfo = RigidInfo()) : self(self), shapeInfo(shapeInfo), rigidInfo(rigidInfo) {
		assert(self.valid());
	};

	void getWorldTransform(btTransform& worldTransform) const final {
		if (!self.has_component<Transform>())
			return;
		
		auto transform = self.component<const Transform>();

		//glm::vec3 globalPosition;
		//glm::quat globalRotation;

		//transform->globalDecomposed(&posiition, &globalRotation, nullptr);

		//if (transform->parent.valid() && transform->parent.has_component<Transform>())
		//	worldTransform.setOrigin(toBt(globalPosition + transform->rotation * rigidInfo.centerOfMass));
		//else
			worldTransform.setOrigin(toBt(transform->position));

		worldTransform.setRotation(toBt(transform->rotation));
	}

	void setWorldTransform(const btTransform& worldTransform) final {
		if (!self.has_component<Transform>())
			return;

		auto transform = self.component<Transform>();

		//if (transform->parent.valid() && transform->parent.has_component<Transform>()) {
		//	glm::vec3 parentGlobalPosition;
		//	glm::quat parentGlobalRotation;
		//
		//	transform->parent.component<Transform>()->globalDecomposed(&parentGlobalPosition, &parentGlobalRotation, nullptr);
		//
		//	transform->position = (fromBt(worldTransform.getOrigin()) - parentGlobalPosition) * parentGlobalRotation;
		//	transform->rotation = glm::inverse(parentGlobalRotation) * fromBt(worldTransform.getRotation());
		//}
		//else {
			//transform->position = fromBt(worldTransform.getOrigin()) - transform->rotation * rigidInfo.centerOfMass;
			transform->position = fromBt(worldTransform.getOrigin());

			transform->rotation = fromBt(worldTransform.getRotation());
		//}
	}
};