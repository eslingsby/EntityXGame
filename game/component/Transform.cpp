#include "component\Transform.hpp"

#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtx\matrix_decompose.hpp>

const glm::vec3 Transform::up(0, 0, 1);
const glm::vec3 Transform::down(0, 0, -1);
const glm::vec3 Transform::left(-1, 0, 0);
const glm::vec3 Transform::right(1, 0, 0);
const glm::vec3 Transform::forward(0, 1, 0);
const glm::vec3 Transform::back(0, -1, 0);

glm::mat4 Transform::localMatrix() const {
	glm::mat4 matrix;

	matrix = glm::translate(matrix, position);
	matrix *= glm::mat4_cast(rotation);
	matrix = glm::scale(matrix, scale);

	return matrix;
}

glm::mat4 Transform::globalMatrix() const {
	glm::mat4 matrix;

	const Transform* transform = this;
	
	do {
		matrix = transform->localMatrix() * matrix;
	
		if (transform->parent.valid() && transform->parent.has_component<Transform>())
			transform = transform->parent.component<const Transform>().get();
		else
			transform = nullptr;
	
	} while (transform);

	return matrix;
}

void Transform::globalDecomposed(glm::vec3* position, glm::quat* rotation, glm::vec3* scale) const {
	glm::vec3 tempPosition;
	glm::quat tempRotation;
	glm::vec3 tempScale;
	
	if (!position)
		position = &tempPosition;
	if (!rotation)
		rotation = &tempRotation;
	if (!scale)
		scale = &tempScale;
	
	glm::decompose(globalMatrix(), *scale, *rotation, *position, glm::vec3(), glm::vec4());

	*rotation = glm::inverse(*rotation); // not sure why, goes crazy without???
}

void Transform::localRotate(const glm::quat& rotate) {
	rotation = rotation * rotate;
}

void Transform::localTranslate(const glm::vec3& translation) {
	position = position + rotation * translation;
}

void Transform::localScale(const glm::vec3 & scaling) {
	scale = scale * scaling;
}

void Transform::globalRotate(const glm::quat& rotate) {
	//if (!parent)
	rotation = rotate * rotation;
	//else
	//	_setRotation(glm::inverse(parent->worldRotation()) * (rotation * worldRotation()));
}

void Transform::globalTranslate(const glm::vec3& translation) {
	//if (!parent)
	position = position + translation;
	//else
	//	_setPosition(_position + glm::inverse(parent->worldRotation()) * translation);
}

void Transform::globalScale(const glm::vec3 & scaling) {
	//if (!parent)
	scale = scale * scaling;
	//else
	//	_setScale(scaling / parent->worldScale());
}