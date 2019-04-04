#pragma once

#include <entityx\entityx.h>
#include "WindowEvents.hpp"

class Game : public entityx::EntityX, public entityx::Receiver<Game> {
	bool _running = true;
	int _error = 0;

	entityx::Entity _camera;
	std::vector<entityx::Entity> _sandbox;

public:
	Game(int argc, char** argv);

	void receive(const WindowFocusEvent& windowFocusEvent);
	void receive(const MousePressEvent& mousePressEvent);
	void receive(const WindowOpenEvent& windowOpenEvent);
	void receive(const KeyInputEvent& keyInputEvent);

	int run();
};