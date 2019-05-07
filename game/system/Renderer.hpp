#pragma once

#include <entityx\System.h>
#include <SDL_events.h>

#include "other\GlLoader.hpp"
#include "other\Line.hpp"

#include "component\Transform.hpp"
#include "component\Model.hpp"
#include "component\Camera.hpp"

#include "system\WindowEvents.hpp"

class Renderer : public entityx::System<Renderer>, public entityx::Receiver<Renderer> {
public:
	struct UniformNames {
		std::string modelMatrix = "model";
		std::string textureSampler = "texture";
		std::string textureScale = "textureScale";
		std::string globalMatricesStruct = "GlobalMatrices";
		//std::string bonesArray = "bones";
	};

	struct ConstructorInfo {
		std::string path = "";
		GlLoader::AttributeInfo attributes;
		UniformNames uniformNames;

		std::string mainVertexShader;
		std::string mainFragmentShader;
		std::string lineVertexShader;
		std::string lineFragmentShader;

		std::string defaultTexture;
	};

private:
	struct GlobalMatrices {
		glm::mat4 view;
		glm::mat4 projection;
	};

	const std::string _path;
	const UniformNames _uniformNames;

	GlLoader _glLoader;

	GlLoader::ProgramContext _mainProgram;
	GlLoader::ProgramContext _lineProgram;

	GlLoader::TextureContext _defaultTexture;
	
	GLuint _uniformBuffer = 0;

	GLuint _lineBufferObject = 0;
	GLuint _lineBuffer = 0;
	uint32_t _lineCount = 0;

	glm::uvec2 _frameBufferSize;
	entityx::Entity _camera;

public:
	Renderer(const ConstructorInfo& constructorInfo);

	void configure(entityx::EventManager &events) final;
	void update(entityx::EntityManager &entities, entityx::EventManager &events, double dt) final;

	void receive(const entityx::ComponentAddedEvent<Model>& modelAddedEvent);
	void receive(const entityx::ComponentAddedEvent<Camera>& cameraAddedEvent);
	void receive(const FramebufferSizeEvent& frameBufferSizeEvent);

	entityx::Entity createScene(entityx::EntityManager &entities, const std::string& meshFile, entityx::Entity entity = entityx::Entity());

	glm::mat4 projectionMatrix() const;
	glm::mat4 viewMatrix() const;

	void rebufferLines(uint32_t count, const Line* lines);
};