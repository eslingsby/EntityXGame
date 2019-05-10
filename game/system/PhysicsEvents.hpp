#pragma once

#include <entityx\Entity.h>

#include <glm\vec3.hpp>

struct ContactEvent {
	struct Contact {
		glm::vec3 globalContactPosition;
		glm::vec3 globalContactNormal;
		float contactDistance = 0.f;
		float contactImpulse = 0.f;
	};

	entityx::Entity firstEntity;
	entityx::Entity secondEntity;

	ContactEvent::Contact contacts[4];
	uint8_t contactCount = 0;
};

struct CollidingEvent : public ContactEvent {
	bool colliding;
};

struct PhysicsUpdateEvent {
	double timestep;
};