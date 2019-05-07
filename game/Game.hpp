#pragma once

#include "Engine.hpp"

class Game : public Engine {
	entityx::Entity _body;
	entityx::Entity _head;

	glm::vec3 _spawnLocation;

	bool _wasHovering = false;

	bool _rDown = false;
	bool _tDown = false;

	std::vector<entityx::Entity> _sandbox;
	std::vector<entityx::Entity> _sounds;
public:
	Game(int argc, char** argv);

	void receive(const MousePressEvent& mousePressEvent) final;
	void receive(const KeyInputEvent& keyInputEvent) final;

	void update(double dt) final;
};