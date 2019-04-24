#pragma once

#include <glm\vec3.hpp>
#include <glm\gtc\quaternion.hpp>
#include <glm\mat4x4.hpp>

#include <entityx\Entity.h>

struct Transform {
	static const glm::vec3 up;
	static const glm::vec3 down;
	static const glm::vec3 left;
	static const glm::vec3 right;
	static const glm::vec3 forward;
	static const glm::vec3 back;

	entityx::Entity parent;

	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale = { 1, 1, 1 };

	glm::mat4 localMatrix() const;
	glm::mat4 globalMatrix() const;
	void globalDecomposed(glm::vec3* position, glm::quat* rotation = nullptr, glm::vec3* scale = nullptr) const;

	void localRotate(const glm::quat& rotation);
	void localTranslate(const glm::vec3& translation);
	void localScale(const glm::vec3 & scaling);

	void globalRotate(const glm::quat& rotation);
	void globalTranslate(const glm::vec3& translation);
	void globalScale(const glm::vec3 & scaling);
};