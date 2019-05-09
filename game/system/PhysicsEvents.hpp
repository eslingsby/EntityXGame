#pragma once

#include <entityx\Entity.h>

#include <glm\vec3.hpp>

struct ContactEvent {
	entityx::Entity firstEntity;
	entityx::Entity secondEntity;

	glm::vec3 globalContactPosition;
	glm::vec3 globalContactNormal;
	float contactDistance = 0.f;
	float contactImpulse = 0.f;
};

struct CollidingEvent {
	entityx::Entity firstEntity;
	entityx::Entity secondEntity;

	bool colliding;
};

struct PhysicsUpdateEvent {
	entityx::EntityManager& entities;
	double dt;
	uint32_t stepCount;
	uint32_t currentStep;
};