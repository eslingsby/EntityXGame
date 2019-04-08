#pragma once

#include <entityx\System.h>

#include "WindowEvents.hpp"
#include "PhysicsEvents.hpp"

class Controller : public entityx::System<Controller>, public entityx::Receiver<Controller> {
	//entityx::Entity _controlled;

	entityx::Entity _head;
	entityx::Entity _body;

	bool _forward = false;
	bool _back = false;
	bool _left = false;
	bool _right = false;

	bool _up = false;
	bool _down = false;

	bool _jumped = false;

	bool _boost = false;

	float _flash = 0;

	glm::vec2 _mousePos;

	bool _locked = false;
	bool _cursorInside = false;

	float _xAngle = 0.f;
	
	bool _enabled = true;

	uint32_t _touchingCount = 0;

public:
	void configure(entityx::EventManager &events) final;
	void update(entityx::EntityManager &es, entityx::EventManager &events, double dt) final;

	void receive(const CursorEnterEvent& cursorEnterEvent);
	void receive(const CursorPositionEvent& cursorPositionEvent);
	void receive(const KeyInputEvent& keyInputEvent);
	void receive(const MousePressEvent& mousePressEvent);
	void receive(const ScrollWheelEvent& scrollWheelEvent);
	void receive(const CollidingEvent& collidingEvent);

	void setEnabled(bool enabled);
	void setControlled(entityx::Entity head, entityx::Entity body);
};