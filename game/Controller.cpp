#include "Controller.hpp"

#include "Transform.hpp"

void Controller::configure(entityx::EventManager &events){
	events.subscribe<CursorEnterEvent>(*this);
	events.subscribe<CursorPositionEvent>(*this);
	events.subscribe<KeyInputEvent>(*this);
	events.subscribe<MousePressEvent>(*this);
	events.subscribe<ScrollWheelEvent>(*this);
}

void Controller::update(entityx::EntityManager &entities, entityx::EventManager &events, double dt){
	if (!_controlled.valid() || !_controlled.has_component<Transform>())
		return;

	auto transform = _controlled.component<Transform>();
	
	transform->globalRotate(glm::quat({ 0.0, 0.0, -_mousePos.x * dt }));
	transform->localRotate(glm::quat({ -_mousePos.y * dt, 0.0, 0.0 }));
	
	float moveSpeed = 300.f * dt;
	
	if (_boost)
		moveSpeed = 1000.f * dt;
	
	transform->localTranslate(glm::vec3(0.f, 0.f, -_flash * 100.f));
	
	if (_forward)
		transform->localTranslate(Transform::localForward * (float)moveSpeed);
	if (_back)
		transform->localTranslate(Transform::localBack * (float)moveSpeed);
	if (_left)
		transform->localTranslate(Transform::localLeft * (float)moveSpeed);
	if (_right)
		transform->localTranslate(Transform::localRight * (float)moveSpeed);
	if (_up)
		transform->globalTranslate(Transform::globalUp * (float)moveSpeed);
	if (_down)
		transform->globalTranslate(Transform::globalDown * (float)moveSpeed);

	_flash = 0;
	_mousePos = { 0.0, 0.0 };
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

void Controller::setControlled(entityx::Entity entity){
	_controlled = entity;
}
