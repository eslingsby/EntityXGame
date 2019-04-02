#include "Renderer.hpp"

#include "Transform.hpp"

#include <experimental\filesystem>
#include <glm\gtc\matrix_transform.hpp>

void errorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
	std::string errorMessage(message, message + length);
	std::cerr << source << ',' << type << ',' << id << ',' << severity << std::endl << errorMessage << std::endl << std::endl;
}

std::string Renderer::_fullPath(const std::string & relativePath) const {
	std::experimental::filesystem::path path = _path;

	path.replace_filename(relativePath);

	return path.string();
}

Renderer::Renderer(const ConstructorInfo& constructorInfo) : 
		_path(constructorInfo.path), 
		_glLoader(constructorInfo.attributes),
		_shape(constructorInfo.defualtShape), 
		_uniformNames(constructorInfo.uniformNames),
		_defaultVertexShader(constructorInfo.defaultVertexShader),
		_defaultFragmentShader(constructorInfo.defaultFragmentShader),
		_defaultTexture(constructorInfo.defaultTexture){

	assert(constructorInfo.path != "");
	assert(constructorInfo.defaultFragmentShader != "");
	assert(constructorInfo.defaultVertexShader != "");
}

void Renderer::configure(entityx::EventManager& events) {
	events.subscribe<entityx::ComponentAddedEvent<Model>>(*this);
	events.subscribe<FramebufferSizeEvent>(*this);

	glDebugMessageCallback(errorCallback, nullptr);

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_DITHER);
	
	setMainProgram(_defaultVertexShader, _defaultFragmentShader);
	setDefaultTexture(_defaultTexture);
}

void Renderer::update(entityx::EntityManager& entities, entityx::EventManager& events, double dt) {
	glm::mat4 projectionMatrix;
	
	if (_shape.verticalFov && _shape.size.x && _shape.size.y && _shape.zDepth)
		projectionMatrix = glm::perspectiveFov(glm::radians(_shape.verticalFov), (float)_shape.size.x, (float)_shape.size.y, 1.f, _shape.zDepth);
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glViewport(0, 0, _shape.size.x, _shape.size.y);
	
	for (auto entity : entities.entities_with_components<Transform, Model>()) {
		const Transform& transform = *entity.component<Transform>().get();
		Model& model = *entity.component<Model>().get();
	
		if (!_mainProgram.program || !model.meshContext.indexCount || !model.textureContext.textureBuffer)
			return;
	
		glUseProgram(_mainProgram.program);
	
		// projection matrix
		if (_mainProgram.projectionUnifLoc != -1)
			glUniformMatrix4fv(_mainProgram.projectionUnifLoc, 1, GL_FALSE, &projectionMatrix[0][0]);
	
		// view matrix
		glm::dmat4 viewMatrix = Renderer::viewMatrix();
	
		if (_mainProgram.viewUnifLoc != -1)
			glUniformMatrix4fv(_mainProgram.viewUnifLoc, 1, GL_FALSE, &((glm::mat4)viewMatrix)[0][0]);
	
		// model matrix
		glm::dmat4 modelMatrix;
	
		if (_mainProgram.modelViewUnifLoc != -1 || _mainProgram.viewUnifLoc != -1) {
			modelMatrix = transform.globalMatrix();
	
			if (_mainProgram.viewUnifLoc != -1)
				glUniformMatrix4fv(_mainProgram.modelUnifLoc, 1, GL_FALSE, &((glm::mat4)modelMatrix)[0][0]);
		}
	
		// model view matrix
		if (_mainProgram.modelViewUnifLoc != -1)
			glUniformMatrix4fv(_mainProgram.modelViewUnifLoc, 1, GL_FALSE, &((glm::mat4)(viewMatrix * modelMatrix))[0][0]);
	
		// texture
		if (_mainProgram.textureUnifLoc != -1) {
			glBindTexture(GL_TEXTURE_2D, model.textureContext.textureBuffer);
	
			glUniform1i(_mainProgram.textureUnifLoc, 0);
		}
	
		// mesh
		glBindVertexArray(model.meshContext.arrayObject);
	
		glBindBuffer(GL_ARRAY_BUFFER, model.meshContext.vertexBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.meshContext.indexBuffer);
	
		glDrawElements(GL_TRIANGLES, model.meshContext.indexCount, GL_UNSIGNED_INT, 0);
	};
}

void Renderer::receive(const entityx::ComponentAddedEvent<Model>& modelAddedEvent){
	auto model = modelAddedEvent.component;

	// Mesh file
	if (model->filePaths.meshFile == "")
		return;

	auto sceneContext = _glLoader.loadMesh(_fullPath(model->filePaths.meshFile));

	if (!sceneContext)
		return;

	if (sceneContext && sceneContext->meshContexts.size() > model->filePaths.meshIndex)
		model->meshContext = sceneContext->meshContexts[model->filePaths.meshIndex];
	else
		return;

	// Texture file
	if (model->filePaths.textureFile != "") {
		auto textureContext = _glLoader.loadTexture(_fullPath(model->filePaths.textureFile));

		if (textureContext)
			model->textureContext = *textureContext;
	}
	else {
		model->textureContext = _defaultTextureContext;
	}
}

void Renderer::receive(const FramebufferSizeEvent& frameBufferSizeEvent){
	_shape.size = frameBufferSizeEvent.size;
}

void Renderer::setCamera(entityx::Entity entity){
	_camera = entity;
}

void Renderer::setMainProgram(const std::string & vertexFile, const std::string & fragmentFile) {
	if (vertexFile == "" || fragmentFile == "")
		return;

	auto programContext = _glLoader.loadProgram(_fullPath(vertexFile), _fullPath(fragmentFile));

	if (!programContext)
		return;

	// Cast to bigger ProgramContext
	_mainProgram = *(ProgramContext*)programContext;

	_mainProgram.modelUnifLoc = glGetUniformLocation(_mainProgram.program, _uniformNames.modelUnifName.c_str());
	_mainProgram.viewUnifLoc = glGetUniformLocation(_mainProgram.program, _uniformNames.viewUnifName.c_str());
	_mainProgram.projectionUnifLoc = glGetUniformLocation(_mainProgram.program, _uniformNames.projectionUnifName.c_str());
	_mainProgram.modelViewUnifLoc = glGetUniformLocation(_mainProgram.program, _uniformNames.modelViewUnifName.c_str());
	_mainProgram.textureUnifLoc = glGetUniformLocation(_mainProgram.program, _uniformNames.textureUnifName.c_str());
}

void Renderer::setDefaultTexture(const std::string & textureFile){
	if (textureFile == "")
		return;

	auto textureContext = _glLoader.loadTexture(_fullPath(textureFile));

	if (!textureContext)
		return;

	_defaultTextureContext = *textureContext;
}

entityx::Entity Renderer::createScene(entityx::EntityManager& entities, const std::string & meshFile, entityx::Entity entity){
	if (meshFile == "")
		return entityx::Entity();
	
	auto sceneContext = _glLoader.loadMesh(_fullPath(meshFile));
	
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

glm::mat4 Renderer::viewMatrix() const {
	if (!_camera.valid() || !_camera.has_component<Transform>())
		return glm::mat4();

	return glm::inverse(_camera.component<const Transform>()->globalMatrix());
}

/*
#include "Transform.hpp"
#include "Name.hpp"

#include <iostream>
#include <filesystem>

#include <glm\gtc\matrix_transform.hpp>


struct Vertex {
	glm::vec3 position;
	glm::vec2 texcoord;
};

struct Texel {
	glm::vec3 position;
	glm::vec3 tangent;
	glm::uvec2 texcoord;
};

inline float crossProduct(const glm::vec2& a, const glm::vec2& b) {
	return glm::cross(glm::vec3(a, 0), glm::vec3(b, 0)).z;
}

inline glm::vec3 baryInterpolate(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, float s, float t) {
	return v1 + s * (v2 - v1) + t * (v3 - v1);
}

template <typename T>
inline void interpolateTexels(Vertex triangle[3], const glm::vec2& resolution, const T& lambda) {
	// order triangle verts by y
	//std::sort(triangle, triangle + 3, [](const Vertex& a, const Vertex& b) -> bool {
	//	return a.texcoord.y < b.texcoord.y;
	//});

	// scale texcoords to resolution
	glm::vec2 vt1 = glm::clamp(triangle[0].texcoord, { 0.f, 0.f }, { 1.f, 1.f }) * resolution;
	glm::vec2 vt2 = glm::clamp(triangle[1].texcoord, { 0.f, 0.f }, { 1.f, 1.f }) * resolution;
	glm::vec2 vt3 = glm::clamp(triangle[2].texcoord, { 0.f, 0.f }, { 1.f, 1.f }) * resolution;

	// find triangle bounding box
	int minX = (int)glm::floor(glm::min(vt1.x, glm::min(vt2.x, vt3.x)));
	int maxX = (int)glm::ceil(glm::max(vt1.x, glm::max(vt2.x, vt3.x)));
	int minY = (int)glm::floor(glm::min(vt1.y, glm::min(vt2.y, vt3.y)));
	int maxY = (int)glm::ceil(glm::max(vt1.y, glm::max(vt2.y, vt3.y)));

	glm::vec2 vs1 = vt2 - vt1; // vt1 vertex to vt2
	glm::vec2 vs2 = vt3 - vt1; // vt1 vertex to vt3

	for (int x = minX; x < maxX; x++) {
		for (int y = minY; y < maxY; y++) {
			glm::vec2 q{ x - vt1.x, y - vt1.y };

			q += glm::vec2{ 0.5f, 0.5f };

			float area2 = crossProduct(vs1, vs2); // area * 2 of triangle

			// find barycentric coordinates
			float s = crossProduct(q, vs2) / area2;
			float t = crossProduct(vs1, q) / area2;

			glm::vec3 position = baryInterpolate(triangle[0].position, triangle[1].position, triangle[2].position, s, t);
			//glm::vec3 normal = baryInterpolate(triangle[0].normal, triangle[1].normal, triangle[2].normal, s, t);

			// if in triangle
			if ((s >= 0) && (t >= 0) && (s + t <= 1))
				lambda({ position, { x, y } });
		}
	}
}
*/

/*
inline void errorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
	std::string errorMessage(message, message + length);
	std::cerr << source << ',' << type << ',' << id << ',' << severity << std::endl << errorMessage << std::endl << std::endl;
}
*/

/*
void Renderer::buildLightmaps(){
	// testing container
	std::vector<std::pair<glm::vec3, glm::quat>> texels;

	for (uint32_t i = 0; i < _meshContexts.size(); i++) {
		// buffer vertex data from opengl
		const MeshContext& meshContext = _meshContexts[i];

		glBindVertexArray(meshContext.arrayObject);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshContext.indexBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, meshContext.vertexBuffer);

		GLuint texcoordsEnabled = 0;
		glGetVertexAttribIuiv(_constructionInfo.texcoordAttrLoc, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &texcoordsEnabled);

		if (!texcoordsEnabled)
			continue;

		size_t texcoordsOffset = 0;
		glGetVertexAttribPointerv(_constructionInfo.texcoordAttrLoc, GL_VERTEX_ATTRIB_ARRAY_POINTER, (void**)&texcoordsOffset);

		uint32_t* indices = (uint32_t*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_READ_ONLY);
		uint8_t* buffer = (uint8_t*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);

		glm::vec3* positions = (glm::vec3*)(buffer);
		glm::vec2* texcoords = (glm::vec2*)(buffer + texcoordsOffset);

		// find all entities with same mesh
		std::vector<uint64_t> entityIds;

		_engine.iterateEntities([&](Engine::Entity& entity) {
			if (!entity.has<Transform, Model>())
				return;

			const Model& model = *entity.get<Model>();

			if (model.meshContextId == i + 1)
				entityIds.push_back(entity.id());
		});
		
		// interpolate texel positions for each entity (due to different lightmap resolutions)
		for (uint64_t id : entityIds) {
			const Transform& transform = *_engine.getComponent<Transform>(id);
			const Model& model = *_engine.getComponent<Model>(id);

			glm::mat4 modelMatrix = transform.globalMatrix();

			// iterate over triangles in mesh
			for (uint32_t y = 0; y < meshContext.indexCount / 3; y++) {
				Vertex triangle[3];

				for (uint8_t x = 0; x < 3; x++) {
					uint32_t index = indices[y * 3 + x];
					triangle[x] = { positions[index], texcoords[index] };
				}

				glm::vec3 tangent = glm::cross(triangle[1].position - triangle[0].position, triangle[2].position - triangle[0].position);

				interpolateTexels(triangle, model.lightmapResolution, [&](const Vertex& vertex) {
					glm::vec3 worldPosition = modelMatrix * glm::vec4(vertex.position, 1);
					glm::quat worldRotation;// = glm::inverse(glm::lookAt(worldPosition, worldPosition + tangent, { 1.f, 1.f, 1.f }));
				
					texels.push_back({ worldPosition, worldRotation });
				});
			}
		}

		// free data from opengl
		glUnmapBuffer(GL_ARRAY_BUFFER);
		glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
	}

	// testing, change this!
	for (auto i : texels) {
		Test::createAxis(_engine, _path, i.first, i.second);
	}
}
*/

/*
void nodeToEntity(Engine& engine, const std::string& meshFile, const GlLoader::MeshHierarchy::Node& node, uint64_t id, bool reload = false) {
	if (!engine.hasComponents<Transform>(id)) {
		Transform* transform = engine.addComponent<Transform>(id);

		transform->position = node.position;
		transform->rotation = node.rotation;
		transform->scale = node.scale;
	}

	if (node.name != "")
		engine.addComponent<Name>(id, node.name);

	if (node.hasMesh) {
		Model* model = engine.addComponent<Model>(id);
		model->loadMesh(meshFile, node.meshContextIndex, reload);
	}
}

void Renderer::_render(glm::uvec2 position, glm::uvec2 size) {
	if (!size.x && !size.y)
		size = _size;
	
	if (_shape.verticalFov && _size.x && _size.y && _shape.zDepth)
		_projectionMatrix = glm::perspectiveFov(glm::radians(_shape.verticalFov), (float)size.x, (float)size.y, 1.f, _shape.zDepth);
	
	glViewport(position.x, position.y, size.x, size.y);
	
	_engine.iterateEntities([&](Engine::Entity& entity) {
		if (!entity.has<Transform, Model>())
			return;
	
		const Transform& transform = *entity.get<Transform>();
		Model& model = *entity.get<Model>();
	
		if (!_mainProgram.programContext.program || !model.meshContext().indexCount || !model.textureContext().textureBuffer)
			return;
	
		glUseProgram(_mainProgram.programContext.program);
	
		// projection matrix
		if (_mainProgram.projectionUnifLoc != -1)
			glUniformMatrix4fv(_mainProgram.projectionUnifLoc, 1, GL_FALSE, &_projectionMatrix[0][0]);
	
		// view matrix
		glm::dmat4 viewMatrix = Renderer::viewMatrix();
	
		if (_mainProgram.viewUnifLoc != -1)
			glUniformMatrix4fv(_mainProgram.viewUnifLoc, 1, GL_FALSE, &((glm::mat4)viewMatrix)[0][0]);
	
		// model matrix
		glm::dmat4 modelMatrix;
	
		if (_mainProgram.modelViewUnifLoc != -1 || _mainProgram.viewUnifLoc != -1) {
			modelMatrix = transform.globalMatrix();
	
			if (_mainProgram.viewUnifLoc != -1)
				glUniformMatrix4fv(_mainProgram.modelUnifLoc, 1, GL_FALSE, &((glm::mat4)modelMatrix)[0][0]);
		}
	
		// model view matrix
		if (_mainProgram.modelViewUnifLoc != -1)
			glUniformMatrix4fv(_mainProgram.modelViewUnifLoc, 1, GL_FALSE, &((glm::mat4)(viewMatrix * modelMatrix))[0][0]);
	
		// texture
		if (_mainProgram.textureUnifLoc != -1) {
			glBindTexture(GL_TEXTURE_2D, model.textureContext().textureBuffer);
		
			glUniform1i(_mainProgram.textureUnifLoc, 0);
		}
	
		// mesh
		glBindVertexArray(model.meshContext().arrayObject);
	
		glBindBuffer(GL_ARRAY_BUFFER, model.meshContext().vertexBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.meshContext().indexBuffer);
	
		glDrawElements(GL_TRIANGLES, model.meshContext().indexCount, GL_UNSIGNED_INT, 0);
	});
}

Renderer::Renderer(Engine& engine, const ShaderVariableInfo & shaderVariables) : _engine(engine), _uniforms(shaderVariables.uniforms), _glLoader(shaderVariables.attributes), _camera(engine) {
	INTERFACE_ENABLE(engine, SystemInterface::initiate)(0);
	INTERFACE_ENABLE(engine, SystemInterface::update)(1);
	INTERFACE_ENABLE(engine, SystemInterface::framebufferSize)(0);
}

void Renderer::initiate(const std::vector<std::string>& args) {
	std::experimental::filesystem::path path = args[0];
	_path = path.replace_filename("data\\").string();

	glDebugMessageCallback(errorCallback, nullptr);

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_DITHER);
}

void Renderer::update(double dt) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	_render({ 0, 0 }, _size);
}

void Renderer::framebufferSize(glm::uvec2 size) {
	_size = size;
}

void Renderer::setMainProgram(const std::string & vertexFile, const std::string & fragmentFile, bool reload){
	const GlLoader::ProgramContext* program = _glLoader.loadProgram(_path + vertexFile, _path + fragmentFile, reload);

	if (program) {
		_mainProgram.programContext = *program;

		_mainProgram.modelUnifLoc = glGetUniformLocation(_mainProgram.programContext.program, _uniforms.modelUnifName.c_str());
		_mainProgram.viewUnifLoc = glGetUniformLocation(_mainProgram.programContext.program, _uniforms.viewUnifName.c_str());
		_mainProgram.projectionUnifLoc = glGetUniformLocation(_mainProgram.programContext.program, _uniforms.projectionUnifName.c_str());
		_mainProgram.modelViewUnifLoc = glGetUniformLocation(_mainProgram.programContext.program, _uniforms.modelViewUnifName.c_str());
		_mainProgram.textureUnifLoc = glGetUniformLocation(_mainProgram.programContext.program, _uniforms.textureUnifName.c_str());
	}
}

void Renderer::setShape(const ShapeInfo& shape) {
	_shape = shape;
}

void Renderer::setCamera(uint64_t id) {
	_camera.set(id);
}

uint64_t Renderer::loadScene(const std::string& meshFile, uint64_t id, bool reload) {
	const GlLoader::MeshHierarchy* scene = _glLoader.loadMesh(_path + meshFile, reload);

	if (!scene || !scene->meshContexts.size())
		return 0;

	if (!id)
		id = _engine.createEntity();

	const auto& root = scene->nodes[0];

	nodeToEntity(_engine, meshFile, root, id);

	std::vector<uint64_t> ids;
	ids.resize(scene->nodes.size());

	ids[0] = id;

	for (uint32_t i = 1; i < scene->nodes.size(); i++) {
		uint64_t currentId = _engine.createEntity();
		ids[i] = currentId;

		const auto& currentNode = scene->nodes[i];

		nodeToEntity(_engine, meshFile, currentNode, currentId);

		uint64_t parentId = ids[currentNode.parentNodeIndex];

		Transform* parentTransform = _engine.getComponent<Transform>(parentId);

		parentTransform->addChild(currentId);
	}
}

glm::mat4 Renderer::viewMatrix() const {
	if (!_camera.valid() || !_camera.has<Transform>())
		return glm::mat4();

	return glm::inverse(_camera.get<Transform>()->globalMatrix());
}

GlLoader& Renderer::loader() {
	return _glLoader;
}

std::string Renderer::path() const{
	return _path;
}

Model::Model(Engine & engine, uint64_t id, const std::string& meshFile, const std::string& textureFile) : ComponentInterface(engine, id) {
	INTERFACE_ENABLE(engine, ComponentInterface::serialize)(0);

	loadMesh(meshFile);
	loadTexture(textureFile);
}

void Model::serialize(BaseReflector& reflector) {
	reflector.buffer("model", "meshFile", &_meshFile);
	reflector.buffer("model", "meshIndex", &_meshIndex);
	reflector.buffer("model", "textureFile", &_textureFile);

	if (reflector.mode == BaseReflector::In) {
		loadMesh(_meshFile, _meshIndex);
		loadTexture(_textureFile);
	}
}

void Model::loadMesh(const std::string& meshFile, uint32_t meshIndex, bool reload) {
	GlLoader& loader = _engine.system<Renderer>().loader();
	const std::string path = _engine.system<Renderer>().path();

	const GlLoader::MeshHierarchy* scene = loader.loadMesh(path + meshFile, reload);

	if (scene && meshIndex < scene->meshContexts.size()) {
		_meshFile = meshFile;
		_meshIndex = meshIndex;
		meshContext = scene->meshContexts[meshIndex];
	}
}

void Model::loadTexture(const std::string & textureFile, bool reload) {
	GlLoader& loader = _engine.system<Renderer>().loader();
	const std::string path  = _engine.system<Renderer>().path();

	const GlLoader::TextureContext* textureContext = loader.loadTexture(path + textureFile, reload);

	if (textureContext) {
		_textureFile = textureFile;
		textureContext = *textureContext;
	}
}

const GlLoader::MeshContext& Model::meshContext() const {
	return meshContext;
}

const GlLoader::TextureContext& Model::textureContext() const {
	return textureContext;
}
*/