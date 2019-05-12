#include "component\Collider.hpp"

#include "component\Name.hpp"

Collider::Collider(ShapeInfo shapeInfo, BodyInfo bodyInfo) : 
	shapeInfo(shapeInfo), 
	bodyInfo(bodyInfo), 
	shapeVariant(0), 
	rigidBody(0, 0, 0) {
};

void Collider::getWorldTransform(btTransform& worldTransform) const {
	if (!self.valid() || !self.has_component<Transform>())
		return;

	auto transform = self.component<const Transform>();

	glm::vec3 globalPosition;
	glm::quat globalRotation;

	transform->globalDecomposed(&globalPosition, &globalRotation);

	worldTransform.setOrigin(toBt(globalPosition));
	worldTransform.setRotation(toBt(globalRotation));
}

void Collider::setWorldTransform(const btTransform& worldTransform) {
	if (!self.valid() || !self.has_component<Transform>())
		return;

	auto transform = self.component<Transform>();

	glm::vec3 globalPosition = fromBt(worldTransform.getOrigin());
	glm::quat globalRotation = fromBt(worldTransform.getRotation());

	glm::mat4 globalMatrix;
	worldTransform.getOpenGLMatrix((btScalar*)&globalMatrix[0]);

	//if (self.has_component<Name>() && self.component<Name>()->name == "test")
	//	printf("setWorldTransform\n");

	if (transform->parent.valid() && transform->parent.has_component<Transform>()) {
		//glm::vec3 parentGlobalPosition;
		//glm::quat parentGlobalRotation;
		//
		//transform->parent.component<Transform>()->globalDecomposed(&parentGlobalPosition, &parentGlobalRotation);
		//
		//transform->position = (globalPosition - parentGlobalPosition) * parentGlobalRotation;
		//transform->rotation = glm::inverse(parentGlobalRotation) * transform->rotation; // working???

		//glm::mat4 parentMatrix = transform->parent.component<Transform>()->globalMatrix();

		//glm::mat4 localMatrix = glm::inverse(parentMatrix) * globalMatrix;
		//glm::mat4 localMatrix = globalMatrix * glm::inverse(parentMatrix);
		//glm::mat4 localMatrix = glm::inverse(globalMatrix) * parentMatrix;
		//glm::mat4 localMatrix = parentMatrix * glm::inverse(globalMatrix);

		//glm::vec3 localPosition;
		//glm::quat localRotation;
		//glm::decompose(localMatrix, glm::vec3(), localRotation, localPosition, glm::vec3(), glm::vec4());
		
		//transform->position = localPosition;
		//transform->rotation = localRotation;
	}
	else {
		transform->position = globalPosition;
		transform->rotation = globalRotation;
	}
}

void Collider::setActive(bool active){
	rigidBody.activate(active);
}

void Collider::setAlwaysActive(bool alwaysActive){
	if (alwaysActive)
		rigidBody.setActivationState(DISABLE_DEACTIVATION);
	else
		rigidBody.setActivationState(ACTIVE_TAG);
}

void Collider::setLinearVelocity(const glm::vec3& velocity){
	rigidBody.setLinearVelocity(toBt(velocity));
}

void Collider::setAngularVelocity(const glm::vec3& velocity) {
	rigidBody.setAngularVelocity(toBt(velocity));
}

void Collider::setLinearFactor(const glm::vec3 & factor){
	rigidBody.setLinearFactor(toBt(factor));
}

void Collider::setAngularFactor(const glm::vec3 & factor){
	rigidBody.setAngularFactor(toBt(factor));
}

void Collider::setFriction(float friction){
	rigidBody.setFriction(friction);
}

void Collider::setRestitution(float restitution){
	rigidBody.setRestitution(restitution);
}

void Collider::setGravity(const glm::vec3 & gravity){
	rigidBody.setGravity(toBt(gravity));
}

glm::vec3 Collider::getLinearVelocity() const{
	return fromBt(rigidBody.getLinearVelocity());
}

glm::vec3 Collider::getAngularVelocity() const{
	return fromBt(rigidBody.getAngularVelocity());
}

float Collider::getInvMass() const{
	return rigidBody.getInvMass();
}

void Collider::applyForce(const glm::vec3 & force){
	rigidBody.applyCentralForce(toBt(force));
}

void Collider::applyImpulse(const glm::vec3 impulse){
	rigidBody.applyCentralImpulse(toBt(impulse));
}
