#pragma once

#include <entityx\entityx.h>

#include "system\WindowEvents.hpp"

#include <glm\vec3.hpp>

class Engine : public entityx::EntityX, public entityx::Receiver<Engine> {
protected:
	bool _running = true;
	int _error = 0;

	bool _wasHovering = false;

public:
	Engine(int argc, char** argv);

	virtual void receive(const WindowFocusEvent& windowFocusEvent);
	virtual void receive(const MousePressEvent& mousePressEvent);
	virtual void receive(const WindowOpenEvent& windowOpenEvent);
	virtual void receive(const KeyInputEvent& keyInputEvent);

	virtual void update(double dt);
	int run();
};