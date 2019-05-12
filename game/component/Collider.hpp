#pragma once

#include <entityx\Entity.h>

#include <glm\gtx\matrix_decompose.hpp>

#include <LinearMath\btMotionState.h>
#include <btBulletDynamicsCommon.h>

#include <optional>
#include <variant>

#include "component\Transform.hpp"

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
		Sphere, // a = diameter
		Box, // a = width, b = height, c = depth
		Cylinder, // a = diameter, b = height
		Capsule, // a = diameter, b = height
		Cone, // a = diameter, b = height
		Plane
	};

	using ShapeVariant = std::variant<
		btSphereShape,
		btBoxShape,
		btCylinderShapeZ,
		btCapsuleShapeZ,
		btConeShapeZ,
		btStaticPlaneShape
	>;

	enum BodyType {
		Solid,
		Trigger,
		Static,
		StaticTrigger,
		Kinematic
	};

	struct ShapeInfo {
		ShapeType type = Sphere;

		float a = 1.f;
		float b = 1.f;
		float c = 1.f;
		float d = 1.f;
	};

	struct BodyInfo {
		BodyType type = Solid;
		float mass = 0.f;

		bool alwaysActive = false;
		bool callbacks = false;

		glm::vec3 defaultLinearFactor = { 1, 1, 1 };
		glm::vec3 defaultAngularFactor = { 1, 1, 1 };

		glm::vec3 startingLinearVelocity;
		glm::vec3 startingAngularVelocity;

		glm::vec3 centerOfMass;

		float defaultRollingFriction = 0.5f;
		float defaultSpinningFriction = 0.5f;
		float defaultFriction = 0.5f;
		float defaultRestitution = 0.5f;

		float defaultAngularDamping = 0.5f;
		float defaultLinearDamping = 0.5f;

		//bool overrideGravity = false;
		//glm::vec3 defaultGravity;
		//float linearThreshold = 1.f;
		//float angularThreshold = 0.8f;
	};

	entityx::Entity self;

	const ShapeInfo shapeInfo;
	const BodyInfo bodyInfo;

	ShapeVariant shapeVariant;
	btRigidBody rigidBody;
	
protected:
	void getWorldTransform(btTransform& worldTransform) const final;
	void setWorldTransform(const btTransform& worldTransform) final;

public:
	Collider(ShapeInfo shapeInfo, BodyInfo bodyInfo = BodyInfo());

	void setActive(bool active);
	void setAlwaysActive(bool alwaysActive);
	void setLinearVelocity(const glm::vec3& velocity);
	void setAngularVelocity(const glm::vec3& velocity);
	void setLinearFactor(const glm::vec3& factor);
	void setAngularFactor(const glm::vec3& factor);
	void setFriction(float friction);
	void setRestitution(float restitution);
	void setGravity(const glm::vec3& gravity);

	glm::vec3 getLinearVelocity() const;
	glm::vec3 getAngularVelocity() const;

	float getInvMass() const;

	void applyForce(const glm::vec3& force);
	void applyImpulse(const glm::vec3 impulse);
};