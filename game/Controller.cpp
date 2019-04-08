#include "Controller.hpp"

#include "Transform.hpp"
#include "Collider.hpp"

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
		!_body.has_component<Transform>())
		return;

	if (!_enabled)
		return;

	auto headTransform = _head.component<Transform>();
	auto bodyTransform = _body.component<Transform>();

	if (headTransform->parent != _body)
		return;
		
	bodyTransform->globalRotate(glm::quat({ 0.0, 0.0, -_mousePos.x * dt }));

	_xAngle -= _mousePos.y * dt;
	_xAngle = glm::clamp(_xAngle, -glm::half_pi<float>(), glm::half_pi<float>());
	
	headTransform->rotation = glm::quat(glm::vec3{ _xAngle, 0, 0 });
	
	float moveSpeed = 2000.f * dt;
	
	if (_boost)
		moveSpeed *= 2;	

	//bodyTransform->localTranslate(Transform::forward * _flash * 100.f);
	
	if (_forward)
		bodyTransform->localTranslate(Transform::forward * (float)moveSpeed);
	if (_back)
		bodyTransform->localTranslate(Transform::back * (float)moveSpeed);
	if (_left)
		bodyTransform->localTranslate(Transform::left * (float)moveSpeed);
	if (_right)
		bodyTransform->localTranslate(Transform::right * (float)moveSpeed);

	//_flash = 0;
	_mousePos = { 0.0, 0.0 };
		
	if (!_touchingCount)
		return;

	if (!_body.has_component<Collider>()) {
		if (_up)
			bodyTransform->localTranslate(Transform::up * (float)moveSpeed);
		if (_down)
			bodyTransform->localTranslate(Transform::down * (float)moveSpeed);

		return;
	}

	if (_up) {
		if (!_jumped) {
			_jumped = true;

			auto collider = _body.component<Collider>();
			collider->rigidBody->setLinearVelocity(btVector3(0, 0, 2000));
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
	_mousePos = cursorPositionEvent.relative;
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
	_flash = scrollWheelEvent.offset.y;
}

void Controller::receive(const CollidingEvent& collidingEvent){
	if (collidingEvent.contactEvent.firstEntity == _body || collidingEvent.contactEvent.secondEntity == _body) {
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
