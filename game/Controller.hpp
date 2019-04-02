#pragma once

#include <entityx\System.h>

#include "WindowEvents.hpp"

class Controller : public entityx::System<Controller>, public entityx::Receiver<Controller> {
	entityx::Entity _controlled;

	bool _forward = false;
	bool _back = false;
	bool _left = false;
	bool _right = false;

	bool _up = false;
	bool _down = false;

	bool _boost = false;

	float _flash = 0;

	glm::vec2 _mousePos;

	bool _locked = false;
	bool _cursorInside = false;

public:
	void configure(entityx::EventManager &events) final;
	void update(entityx::EntityManager &es, entityx::EventManager &events, double dt) final;

	void receive(const CursorEnterEvent& cursorEnterEvent);
	void receive(const CursorPositionEvent& cursorPositionEvent);
	void receive(const KeyInputEvent& keyInputEvent);
	void receive(const MousePressEvent& mousePressEvent);
	void receive(const ScrollWheelEvent& scrollWheelEvent);

	void setControlled(entityx::Entity entity);
};