#pragma once

#include <entityx\System.h>
#include "WindowEvents.hpp"

class Interface : public entityx::System<Interface>, public entityx::Receiver<Interface> {
	bool _running = false;
	bool _input = true;

	entityx::Entity _focusedEntity;

public:
	Interface();
	~Interface();

	void configure(entityx::EventManager &events) final;
	void update(entityx::EntityManager &entities, entityx::EventManager &events, double dt) final;

	void receive(const WindowOpenEvent& windowOpenEvent);
	void receive(const FramebufferSizeEvent& frameBufferSizeEvent);

	bool isHovering() const;
	void setInputEnabled(bool enabled);
	void setFocusedEntity(entityx::Entity entity);
};