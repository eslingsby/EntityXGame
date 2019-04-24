#include "other\BulletDebug.hpp"

void BulletDebug::drawLine(const btVector3 & from, const btVector3 & to, const btVector3 & color) {
	Line line;

	line.fromPosition = { from.x(), from.y(), from.z() };
	line.fromColour = { color.x(), color.y(), color.z() };

	line.toPosition = { to.x(), to.y(), to.z() };
	line.toColour = { color.x(), color.y(), color.z() };

	_lineBuffer.push_back(line);
}

void BulletDebug::drawLine(const btVector3& from, const btVector3& to, const btVector3& fromColor, const btVector3& toColor) {
	Line line;

	line.fromPosition = { from.x(), from.y(), from.z() };
	line.fromColour = { fromColor.x(), fromColor.y(), fromColor.z() };

	line.toPosition = { to.x(), to.y(), to.z() };
	line.toColour = { toColor.x(), toColor.y(), toColor.z() };

	_lineBuffer.push_back(line);
}


void BulletDebug::drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime, const btVector3 &color) {

}

void BulletDebug::reportErrorWarning(const char *warningString) { }

void BulletDebug::draw3dText(const btVector3 &location, const char *textString) { }

void BulletDebug::setDebugMode(int debugMode) { }

int BulletDebug::getDebugMode() const { 
	return DBG_MAX_DEBUG_DRAW_MODE;
}

void BulletDebug::clearLines(){
	_lineBuffer.clear();
}

const Line * BulletDebug::getLines() const {
	if (!_lineBuffer.size())
		return nullptr;

	return &_lineBuffer[0];
}

uint32_t BulletDebug::lineCount() const {
	return _lineBuffer.size();
}