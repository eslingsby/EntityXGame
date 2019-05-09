#pragma once

#include "Engine.hpp"
#include "system\PhysicsEvents.hpp"

class Game : public Engine {
	entityx::Entity _body;
	entityx::Entity _head;

	glm::vec3 _spawnLocation;

	std::vector<entityx::Entity> _spinners;
	std::vector<entityx::Entity> _sandbox;
public:
	Game(int argc, char** argv);

	void receive(const PhysicsUpdateEvent& physicsEvent);
	void receive(const MousePressEvent& mousePressEvent) final;
	void receive(const KeyInputEvent& keyInputEvent) final;

	void update(double dt) final;
};