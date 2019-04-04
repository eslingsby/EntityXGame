#include "Controller.hpp"

#include "Transform.hpp"
#include "Collider.hpp"

void Controller::_keepUpright(){
	if (!_controlled.has_component<Transform>() || !_controlled.has_component<Collider>())
		return;

	auto transform = _controlled.component<Transform>();
	auto collider = _controlled.component<Collider>();

	collider->rigidBody->activate();

	glm::vec3 eulerAngles = glm::eulerAngles(transform->rotation);

	eulerAngles.x = 0;
	eulerAngles.y = 0;

	transform->rotation = glm::quat(eulerAngles);
}

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

	transform->localTranslate(Transform::forward * _flash * 100.f);
	
	if (_forward)
		transform->localTranslate(Transform::forward * (float)moveSpeed);
	if (_back)
		transform->localTranslate(Transform::back * (float)moveSpeed);
	if (_left)
		transform->localTranslate(Transform::left * (float)moveSpeed);
	if (_right)
		transform->localTranslate(Transform::right * (float)moveSpeed);
	if (_up)
		transform->globalTranslate(Transform::up * (float)moveSpeed);
	if (_down)
		transform->globalTranslate(Transform::down * (float)moveSpeed);

	_flash = 0;
	_mousePos = { 0.0, 0.0 };

	_keepUpright();
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

void Controller::receive(const PhysicsUpdateEvent& physicsUpdateEvent) {
	_keepUpright();
}

void Controller::setControlled(entityx::Entity entity){
	_controlled = entity;
}
