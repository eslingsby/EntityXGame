#include "system\Controller.hpp"

#include "component\Transform.hpp"
#include "component\Collider.hpp"

#include "other\GlmPrint.hpp"

Controller::Controller(const ConstructorInfo & constructorInfo) :
	_enabled(constructorInfo.defaultEnabled), 
	_walkSpeed(constructorInfo.defaultWalkSpeed),
	_runSpeed(constructorInfo.defaultRunSpeed),
	_jumpMagnitude(constructorInfo.defaultJumpMagnitude),
	_mouseSensisitivity(constructorInfo.defaultMouseSensisitivity){
}

void Controller::configure(entityx::EventManager &events){
	events.subscribe<CursorEnterEvent>(*this);
	events.subscribe<CursorPositionEvent>(*this);
	events.subscribe<KeyInputEvent>(*this);
	events.subscribe<MousePressEvent>(*this);
	events.subscribe<ScrollWheelEvent>(*this);
	events.subscribe<CollidingEvent>(*this);
}

void Controller::update(entityx::EntityManager &entities, entityx::EventManager &events, double dt){
	if (!_head.valid() ||
		!_head.has_component<Transform>() ||
		!_body.valid() || 
		!_body.has_component<Transform>() ||
		!_body.has_component<Collider>())
		return;

	if (!_enabled) {
		auto collider = _body.component<Collider>();
		
		glm::vec3 velocity = collider->getLinearVelocity();
		glm::vec3 translation;

		if (_lastTranslation != glm::vec3())
			translation = _lastTranslation /  (float)dt;

		if (_touchingCount)
			translation = glm::mix(translation, glm::vec3(), dt * 10);
		else
			translation = glm::mix(translation, glm::vec3(), dt * 1);

		_lastTranslation = translation * (float)dt;

		collider->setLinearVelocity(translation + (velocity * Transform::up));
		collider->setAngularVelocity(glm::vec3());
		return;
	}

	auto headTransform = _head.component<Transform>();
	auto bodyTransform = _body.component<Transform>();

	if (headTransform->parent != _body)
		return;
		
	_mousePos = glm::mix(_mousePos, _dMousePos, 50 * dt);
	_dMousePos = { 0, 0 };

	_xAngle -= _mousePos.y * dt;
	_xAngle = glm::clamp(_xAngle, -glm::half_pi<float>(), glm::half_pi<float>());
	
	headTransform->rotation = glm::quat(glm::vec3{ _xAngle, 0, 0 });
	
	float moveSpeed = _walkSpeed;
	
	if (_boost)
		moveSpeed = _runSpeed;

	glm::quat globalRotation;
	bodyTransform->globalDecomposed(nullptr, &globalRotation);

	glm::vec3 translation;
	
	if (_forward)
		translation += Transform::forward;
	if (_back)
		translation += Transform::back;
	if (_left)
		translation += Transform::left;
	if (_right)
		translation += Transform::right;

	if (translation != glm::vec3{ 0,0,0 })
		translation = globalRotation * glm::normalize(translation) * moveSpeed * (float)dt;

	if (_touchingCount)
		translation = glm::mix(_lastTranslation, translation, dt * 10);
	else
		translation = glm::mix(_lastTranslation, translation, dt * 1);

	// character controller (overrides velocity, doesn't ride on platforms)
	auto collider = _body.component<Collider>();

	glm::vec3 velocity = collider->getLinearVelocity();

	collider->setLinearVelocity(translation / (float)dt + velocity * Transform::up);

	collider->setAngularVelocity(glm::vec3{ 0.f, 0.f, -_mousePos.x });
	
	_lastTranslation = translation;
		
	if (!_touchingCount)
		return;

	if (_down && !_crouched) {
		bodyTransform->scale.z *= 0.5;
		bodyTransform->position.z -= 35;

		_crouched = true;
	}
	else if (!_down && _crouched){
		bodyTransform->scale.z *= 2;
		bodyTransform->position.z += 35;

		_crouched = false;
	}


	if (_up) {
		if (!_jumped) {
			_jumped = true;

			collider->applyImpulse(Transform::up * _jumpMagnitude);
		}
	}
	else if (!_up && _jumped) {
		_jumped = false;
	}
}

void Controller::receive(const CursorEnterEvent& cursorEnterEvent){
	_cursorInside = cursorEnterEvent.entered;
}

void Controller::receive(const CursorPositionEvent& cursorPositionEvent){
	_dMousePos = cursorPositionEvent.relative;
}

void Controller::receive(const KeyInputEvent& keyInputEvent){
	bool value;

	if (keyInputEvent.action == Action::Press)
		value = true;
	else if (keyInputEvent.action == Action::Release)
		value = false;
	else
		return;

	switch (keyInputEvent.key) {
	case Key::Key_W:
		_forward = value;
		return;
	case Key::Key_S:
		_back = value;
		return;
	case Key::Key_A:
		_left = value;
		return;
	case Key::Key_D:
		_right = value;
		return;
	case Key::Key_Space:
		_up = value;
		return;
	case Key::Key_LCtrl:
		_down = value;
		return;
	case Key::Key_LShift:
		_boost = value;
		return;
	}
}

void Controller::receive(const MousePressEvent& mousePressEvent){

}

void Controller::receive(const ScrollWheelEvent& scrollWheelEvent){

}

void Controller::receive(const CollidingEvent& collidingEvent){
	if (collidingEvent.firstEntity == _body || collidingEvent.secondEntity == _body) {
		if (collidingEvent.colliding)
			_touchingCount++;
		else
			_touchingCount--;
	}
}

void Controller::setEnabled(bool enabled){
	_enabled = enabled;
}

void Controller::setControlled(entityx::Entity head, entityx::Entity body){
	_head = head;
	_body = body;
}
