#pragma once

#include <LinearMath\btIDebugDraw.h>

#include <vector>

#include <glm\vec3.hpp>

#include "other\Line.hpp"

class BulletDebug : public btIDebugDraw {
	std::vector<Point> _pointBuffer;
	std::vector<Line> _lineBuffer;

	void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) final;
	void drawLine(const btVector3& from, const btVector3& to, const btVector3& fromColor, const btVector3& toColor) final;
	void drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime, const btVector3 &color) final;

	void reportErrorWarning(const char *warningString) final;
	void draw3dText(const btVector3 &location, const char *textString) final;
	void setDebugMode(int debugMode) final;
	int getDebugMode() const final;

	void clearLines() final;

public:
	const Line* getLines() const;
	uint32_t lineCount() const;

	friend class Physics;
	friend class btDynamicsWorld;
	friend class btCollisionWorld;
};