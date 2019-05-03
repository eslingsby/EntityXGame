#pragma once

#include "glm\vec3.hpp"

struct Point {
	glm::vec3 position;
	glm::vec3 colour;
};

struct Line {
	Point from;
	Point to;
};