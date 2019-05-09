#include <iostream>
#include <glm\glm.hpp>
#include <glm\gtc\quaternion.hpp>

std::ostream& operator<<(std::ostream& stream, const glm::vec2& vec) {
	return stream << vec.x << ", " << vec.y;
}

std::ostream& operator<<(std::ostream& stream, const glm::vec3& vec) {
	return stream << vec.x << ", " << vec.y << ", " << vec.z;
}

std::ostream& operator<<(std::ostream& stream, const glm::vec4& vec) {
	return stream << vec.w << ", " << vec.x << ", " << vec.y << ", " << vec.z;
}

std::ostream& operator<<(std::ostream& stream, const glm::quat& quat) {
	return stream << glm::radians(glm::eulerAngles(quat));
}