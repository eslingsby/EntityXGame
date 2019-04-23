#pragma once

#include <glm\vec3.hpp>
#include <glm\gtc\quaternion.hpp>

struct Camera {
	glm::vec3 offsetPosition;
	glm::quat offsetRotation;
	float verticalFov = 90.f;
	float zDepth = 1000000.f;
};