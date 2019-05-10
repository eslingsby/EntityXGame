#include <iostream>
#include <glm\glm.hpp>
#include <glm\gtc\quaternion.hpp>

template <typename T>
inline std::ostream& operator<<(std::ostream& stream, const glm::tvec2<T>& vec) {
	return stream << vec.x << ", " << vec.y;
}

template <typename T>
inline std::ostream& operator<<(std::ostream& stream, const glm::tvec3<T>& vec) {
	return stream << vec.x << ", " << vec.y << ", " << vec.z;
}

template <typename T>
inline std::ostream& operator<<(std::ostream& stream, const glm::tvec4<T>& vec) {
	return stream << vec.w << ", " << vec.x << ", " << vec.y << ", " << vec.z;
}

template <typename T>
inline std::ostream& operator<<(std::ostream& stream, const glm::tquat<T>& quat) {
	return stream << glm::radians(glm::eulerAngles(quat));
}