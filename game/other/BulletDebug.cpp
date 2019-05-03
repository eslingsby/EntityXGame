#include "other\BulletDebug.hpp"

#include <iostream>

void BulletDebug::drawLine(const btVector3 & from, const btVector3 & to, const btVector3 & color) {
	Line line;
	line.from = { { from.x(), from.y(), from.z() }, { color.x(), color.y(), color.z() } };
	line.to = { { to.x(), to.y(), to.z() }, { color.x(), color.y(), color.z() } };

	_lineBuffer.push_back(line);
}

void BulletDebug::drawLine(const btVector3& from, const btVector3& to, const btVector3& fromColor, const btVector3& toColor) {
	Line line;
	line.from = { { from.x(), from.y(), from.z() },{ fromColor.x(), fromColor.y(), fromColor.z() } };
	line.to = { { to.x(), to.y(), to.z() },{ toColor.x(), toColor.y(), toColor.z() } };

	_lineBuffer.push_back(line);
}

void BulletDebug::drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime, const btVector3 &color) {
	Point point = { { PointOnB.x(), PointOnB.y(), PointOnB.z() },{ color.x(), color.y(), color.z() } };

	_pointBuffer.push_back(point);
}

void BulletDebug::reportErrorWarning(const char *warningString) { 
	std::cerr << warningString << std::endl;
}

void BulletDebug::draw3dText(const btVector3 &location, const char *textString) { }

void BulletDebug::setDebugMode(int debugMode) { }

int BulletDebug::getDebugMode() const { 
	return DBG_MAX_DEBUG_DRAW_MODE;
}

void BulletDebug::clearLines(){
	_lineBuffer.clear();
	_pointBuffer.clear();
}

const Line * BulletDebug::getLines() const {
	if (!_lineBuffer.size())
		return nullptr;

	return &_lineBuffer[0];
}

uint32_t BulletDebug::lineCount() const {
	return _lineBuffer.size();
}