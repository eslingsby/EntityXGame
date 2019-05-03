#pragma once

#include <entityx\entityx.h>

#include "system\WindowEvents.hpp"

#include <glm\vec3.hpp>

class Game : public entityx::EntityX, public entityx::Receiver<Game> {
	bool _running = true;
	int _error = 0;

	entityx::Entity _body;
	entityx::Entity _head;

	glm::vec3 _spawnLocation;

	bool _wasHovering = false;

	bool _rDown = false;
	bool _tDown = false;

	std::vector<entityx::Entity> _sandbox;
public:
	Game(int argc, char** argv);

	void receive(const WindowFocusEvent& windowFocusEvent);
	void receive(const MousePressEvent& mousePressEvent);
	void receive(const WindowOpenEvent& windowOpenEvent);
	void receive(const KeyInputEvent& keyInputEvent);

	int run();
};