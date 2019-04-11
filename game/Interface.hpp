#pragma once

#include <entityx\System.h>

class Interface : public entityx::System<Interface>, public entityx::Receiver<Interface> {

public:
	Interface();
	~Interface();

	void configure(entityx::EventManager &events) final;
	void update(entityx::EntityManager &entities, entityx::EventManager &events, double dt) final;
};