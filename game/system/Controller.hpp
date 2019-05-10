#pragma once

#include <entityx\System.h>

#include "system\WindowEvents.hpp"
#include "system\PhysicsEvents.hpp"

class Controller : public entityx::System<Controller>, public entityx::Receiver<Controller> {
	entityx::Entity _head;
	entityx::Entity _body;

	float _walkSpeed;
	float _runSpeed;
	float _jumpMagnitude;
	float _mouseSensisitivity;

	bool _forward = false;
	bool _back = false;
	bool _left = false;
	bool _right = false;

	bool _up = false;
	bool _down = false;

	bool _crouched = false;
	bool _jumped = false;

	bool _boost = false;


	glm::vec2 _dMousePos;
	glm::vec2 _mousePos;

	bool _locked = false;
	bool _cursorInside = false;

	float _xAngle = 0.f;
	
	bool _enabled = true;

	uint32_t _touchingCount = 0;

	glm::vec3 _lastTranslation;

public:
	struct ConstructorInfo {
		bool defaultEnabled = true;
		float defaultWalkSpeed = 500.f;
		float defaultRunSpeed = 750.f;
		float defaultJumpMagnitude = 30000.f;
		float defaultMouseSensisitivity = 0.5f;
	};

public:
	Controller(const ConstructorInfo& constructorInfo = ConstructorInfo());

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