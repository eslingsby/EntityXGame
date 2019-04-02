#pragma once

#include <glm\vec3.hpp>
#include <glm\gtc\quaternion.hpp>
#include <glm\mat4x4.hpp>

#include <entityx\Entity.h>

class Transform {
public:
	static const glm::vec3 localUp;
	static const glm::vec3 localDown;
	static const glm::vec3 localLeft;
	static const glm::vec3 localRight;
	static const glm::vec3 localForward;
	static const glm::vec3 localBack;

	static const glm::vec3 globalUp;
	static const glm::vec3 globalDown;
	static const glm::vec3 globalLeft;
	static const glm::vec3 globalRight;
	static const glm::vec3 globalForward;
	static const glm::vec3 globalBack;

	entityx::Entity parent;

	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale = { 1, 1, 1 };

	glm::mat4 localMatrix() const;
	glm::mat4 globalMatrix() const;
	void globalDecomposed(glm::vec3* position, glm::quat* rotation, glm::vec3* scale) const;

	void localRotate(const glm::quat& rotation);
	void localTranslate(const glm::vec3& translation);
	void localScale(const glm::vec3 & scaling);

	void globalRotate(const glm::quat& rotation);
	void globalTranslate(const glm::vec3& translation);
	void globalScale(const glm::vec3 & scaling);
};

/*
class Transform : public ComponentInterface {
	//Engine& _engine;
	//const uint64_t _id;

	uint64_t _parent = 0;

	uint64_t _firstChild = 0;

	uint64_t _leftSibling = _id;
	uint64_t _rightSibling = _id;

public:
	static const glm::vec3 localUp;
	static const glm::vec3 localDown;
	static const glm::vec3 localLeft;
	static const glm::vec3 localRight;
	static const glm::vec3 localForward;
	static const glm::vec3 localBack;

	static const glm::vec3 globalUp;
	static const glm::vec3 globalDown;
	static const glm::vec3 globalLeft;
	static const glm::vec3 globalRight;
	static const glm::vec3 globalForward;
	static const glm::vec3 globalBack;

	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale = { 1, 1, 1 };

private:
	void _getChildren(uint64_t id, uint32_t count, std::vector<uint64_t>* ids) const;

public:
	Transform(Engine& engine, uint64_t id);
	~Transform();

	void serialize(BaseReflector& reflector) final;

	void addChild(uint64_t id);
	void removeParent();
	void removeChildren();

	bool hasChildren() const;
	void getChildren(std::vector<uint64_t>* ids) const;

	glm::mat4 localMatrix() const;
	glm::mat4 globalMatrix() const;

	void localRotate(const glm::quat& rotation);
	void localTranslate(const glm::vec3& translation);
	void localScale(const glm::vec3 & scaling);

	void globalRotate(const glm::quat& rotation);
	void globalTranslate(const glm::vec3& translation);
	void globalScale(const glm::vec3 & scaling);
};
*/