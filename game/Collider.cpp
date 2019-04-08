#include "Collider.hpp"

Collider::Collider(ShapeInfo shapeInfo, BodyInfo bodyInfo) : shapeInfo(shapeInfo), bodyInfo(bodyInfo) { };

void Collider::getWorldTransform(btTransform& worldTransform) const {
	if (!self.has_component<Transform>())
		return;

	auto transform = self.component<const Transform>();

	glm::vec3 globalPosition;
	glm::quat globalRotation;

	transform->globalDecomposed(&globalPosition, &globalRotation);

	worldTransform.setOrigin(toBt(globalPosition));
	worldTransform.setRotation(toBt(globalRotation));
}

void Collider::setWorldTransform(const btTransform& worldTransform) {
	if (!self.has_component<Transform>())
		return;

	auto transform = self.component<Transform>();

	glm::vec3 globalPosition = fromBt(worldTransform.getOrigin());
	glm::quat globalRotation = fromBt(worldTransform.getRotation());

	if (transform->parent.valid() && transform->parent.has_component<Transform>()) {
		glm::vec3 parentGlobalPosition;
		glm::quat parentGlobalRotation;
		
		transform->parent.component<Transform>()->globalDecomposed(&parentGlobalPosition, &parentGlobalRotation);
		
		transform->position = (globalPosition - parentGlobalPosition) * parentGlobalRotation;
		//transform->rotation = glm::inverse(parentGlobalRotation) * transform->rotation; // Colliders as children need fixing!
	}
	else {
		transform->position = globalPosition;
		transform->rotation = globalRotation;
	}
}

void Collider::setActive(bool active){
	assert(rigidBody);
	rigidBody->activate(active);
}

void Collider::setAlwaysActive(bool alwaysActive){
	assert(rigidBody);
	if (alwaysActive)
		rigidBody->setActivationState(DISABLE_DEACTIVATION);
	else
		rigidBody->setActivationState(ACTIVE_TAG);
}

void Collider::setLinearVelocity(const glm::vec3& velocity){
	assert(rigidBody);
	rigidBody->setLinearVelocity(toBt(velocity));
}

void Collider::setAngularVelocity(const glm::vec3& velocity) {
	assert(rigidBody);
	rigidBody->setAngularVelocity(toBt(velocity));
}

void Collider::setLinearFactor(const glm::vec3 & factor){
	assert(rigidBody);
	rigidBody->setLinearFactor(toBt(factor));
}

void Collider::setAngularFactor(const glm::vec3 & factor){
	assert(rigidBody);
	rigidBody->setAngularFactor(toBt(factor));
}