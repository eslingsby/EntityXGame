#include "system\Renderer.hpp"

#include "component\Transform.hpp"

#include "other\Path.hpp"

#include <experimental\filesystem>
#include <glm\gtc\matrix_transform.hpp>

void errorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
	std::string errorMessage(message, message + length);
	std::cerr << "Renderer opengl: " << source << ',' << type << ',' << id << ',' << severity << std::endl << errorMessage << std::endl << std::endl;
}

Renderer::Renderer(const ConstructorInfo& constructorInfo) : 
		_path(constructorInfo.path), 
		_glLoader(constructorInfo.attributes),
		_uniformNames(constructorInfo.uniformNames){

	assert(constructorInfo.path != "");
	assert(constructorInfo.mainVertexShader != "");
	assert(constructorInfo.mainFragmentShader != "");

	// configure OpenGL
	glDebugMessageCallback((GLDEBUGPROC)errorCallback, nullptr);

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_DITHER);

	// load default texture
	auto defaultTexture = _glLoader.loadTexture(formatPath(_path, constructorInfo.defaultTexture));

	if (defaultTexture)
		_defaultTexture = *defaultTexture;

	// load shader programs
	auto mainProgram = _glLoader.loadProgram(formatPath(_path, constructorInfo.mainVertexShader), formatPath(_path, constructorInfo.mainFragmentShader));
	auto lineProgram = _glLoader.loadProgram(formatPath(_path, constructorInfo.lineVertexShader), formatPath(_path, constructorInfo.lineFragmentShader));

	if (mainProgram)
		_mainProgram = *mainProgram;
	
	if (lineProgram)
		_lineProgram = *lineProgram;
	
	// Create line buffer / line program
	glGenVertexArrays(1, &_lineBufferObject);
	glGenBuffers(1, &_lineBuffer);

	// Create uniform buffer
	glGenBuffers(1, &_uniformBuffer);
	glBindBuffer(GL_UNIFORM_BUFFER, _uniformBuffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(GlobalMatrices), nullptr, GL_STREAM_DRAW);
}

void Renderer::configure(entityx::EventManager& events) {
	events.subscribe<entityx::ComponentAddedEvent<Model>>(*this);
	events.subscribe<entityx::ComponentAddedEvent<Camera>>(*this);
	events.subscribe<WindowSizeEvent>(*this);
}

void Renderer::rebufferLines(uint32_t count, const Line * lines){
	if (!count || !lines)
		return;

	glBindVertexArray(_lineBufferObject);
	glBindBuffer(GL_ARRAY_BUFFER, _lineBuffer);

	glBufferData(GL_ARRAY_BUFFER, count * sizeof(Line), lines, GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(0); // position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3) * 2, (void*)(0));

	glEnableVertexAttribArray(1); // colour
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3) * 2, (void*)(sizeof(glm::vec3)));

	_lineCount = count;
}

void Renderer::update(entityx::EntityManager& entities, entityx::EventManager& events, double dt) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (_windowSize == glm::uvec2{ 0, 0 })
		return;

	glViewport(0, 0, _windowSize.x, _windowSize.y);

	// bind shared matrices
	const GlobalMatrices uniforms = { viewMatrix(), projectionMatrix() };

	glBindBuffer(GL_UNIFORM_BUFFER, _uniformBuffer);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(GlobalMatrices), &uniforms);
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, _uniformBuffer, 0, sizeof(GlobalMatrices));

	// draw lines
	if (_lineProgram.program) {
		glUseProgram(_lineProgram.program);
		glUniformBlockBinding(_lineProgram.program, glGetUniformBlockIndex(_lineProgram.program, _uniformNames.globalMatricesStruct.c_str()), 0);

		//glPointSize(10);

		glBindVertexArray(_lineBufferObject);
		glDrawArrays(GL_LINES, 0, _lineCount * 4);
	}

	// draw meshes
	if (!_mainProgram.program)
		return;	

	glUseProgram(_mainProgram.program);
	glUniformBlockBinding(_mainProgram.program, glGetUniformBlockIndex(_mainProgram.program, _uniformNames.globalMatricesStruct.c_str()), 0);

	GLint modelLocation = glGetUniformLocation(_mainProgram.program, _uniformNames.modelMatrix.c_str());
	GLint textureLocation = glGetUniformLocation(_mainProgram.program, _uniformNames.textureSampler.c_str());
	GLint textureScaleLocation = glGetUniformLocation(_mainProgram.program, _uniformNames.textureScale.c_str());

	for (auto entity : entities.entities_with_components<Transform, Model>()) {
		const Transform& transform = *entity.component<Transform>().get();
		const Model& model = *entity.component<Model>().get();
	
		if (!model.meshContext.indexCount || !model.textureContext.textureBuffer)
			continue;
	
		// bind model matrix
		if (modelLocation != -1)
			glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &transform.globalMatrix()[0][0]);
	
		// bind texture
		if (textureLocation != -1) {
			glBindTexture(GL_TEXTURE_2D, model.textureContext.textureBuffer);
			glUniform1i(textureLocation, 0);
		}

		if (textureScaleLocation != -1)
			glUniform2fv(textureScaleLocation, 1, &model.textureScale[0]);
	
		// draw
		glBindVertexArray(model.meshContext.arrayObject);
		glDrawElements(GL_TRIANGLES, model.meshContext.indexCount, GL_UNSIGNED_INT, 0);
	};
}

void Renderer::receive(const entityx::ComponentAddedEvent<Model>& modelAddedEvent){
	auto model = modelAddedEvent.component;

	// Mesh file
	if (model->filePaths.meshFile == "")
		return;

	auto sceneContext = _glLoader.loadMesh(formatPath(_path, model->filePaths.meshFile));

	if (!sceneContext)
		return;

	if (sceneContext && sceneContext->meshContexts.size() > model->filePaths.meshIndex)
		model->meshContext = sceneContext->meshContexts[model->filePaths.meshIndex];
	else
		return;

	// Texture file
	if (model->filePaths.textureFile != "") {
		auto textureContext = _glLoader.loadTexture(formatPath(_path, model->filePaths.textureFile));

		if (textureContext)
			model->textureContext = *textureContext;
	}
	else {
		model->textureContext = _defaultTexture;
	}
}

void Renderer::receive(const entityx::ComponentAddedEvent<Camera>& cameraAddedEvent) {
	_camera = cameraAddedEvent.entity;
}

void Renderer::receive(const WindowSizeEvent & windowSizeEvent){
	_windowSize = windowSizeEvent.size;
}

entityx::Entity Renderer::createScene(entityx::EntityManager& entities, const std::string & meshFile, entityx::Entity entity){
	if (meshFile == "")
		return entityx::Entity();
	
	auto sceneContext = _glLoader.loadMesh(formatPath(_path, meshFile));
	
	if (!sceneContext)
		return entityx::Entity();
	
	if (!entity.valid())
		entity = entities.create();

	// Children list
	std::vector<entityx::Entity> children;
	children.resize(sceneContext->nodes.size());

	children[0] = entity;
	
	for (uint32_t i = 1; i < children.size(); i++)
		children[i] = entities.create();

	// Process root
	entityx::ComponentHandle<Transform> rootTransform;
	
	if (entity.has_component<Transform>())
		rootTransform = entity.component<Transform>();
	else
		rootTransform = entity.assign<Transform>();
	
	auto rootNode = sceneContext->nodes[0];

	if (rootNode.hasMesh)
		entity.assign<Model>(Model::FilePaths{ meshFile, rootNode.meshContextIndex });
	
	// Process children
	for (uint32_t i = 1; i < sceneContext->nodes.size(); i++) {
		const auto node = sceneContext->nodes[i];

		auto transform = children[i].assign<Transform>();
		transform->parent = children[node.parentNodeIndex];

		transform->position = node.position;
		transform->rotation = node.rotation;
		transform->scale = node.scale;

		if (node.hasMesh)
			children[i].assign<Model>(Model::FilePaths{ meshFile, node.meshContextIndex });
	}
	
	return entity;
}

glm::mat4 Renderer::projectionMatrix() const{
	if (!_camera.valid() || !_camera.has_component<Camera>() || _windowSize.x == 0 || _windowSize.y == 0)
		return glm::mat4();

	auto camera = _camera.component<const Camera>();
	return glm::perspectiveFov(glm::radians(camera->verticalFov), (float)_windowSize.x, (float)_windowSize.y, 1.f, camera->zDepth);
}

glm::mat4 Renderer::viewMatrix() const {
	if (!_camera.valid() || !_camera.has_component<Transform>() || !_camera.has_component<Camera>())
		return glm::mat4();

	auto transform = _camera.component<const Transform>();
	auto camera = _camera.component<const Camera>();

	glm::vec3 globalPosition;
	glm::quat globalRotation;

	transform->globalDecomposed(&globalPosition, &globalRotation);

	glm::mat4 view;
	view = glm::translate(view, globalPosition);
	view *= glm::mat4_cast(globalRotation);

	glm::mat4 offset;
	offset = glm::translate(offset, camera->offsetPosition);
	offset *= glm::mat4_cast(camera->offsetRotation);

	return glm::inverse(view * offset);
}