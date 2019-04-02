#include "GlLoader.hpp"

#include <glm\gtx\matrix_decompose.hpp>

#include <assimp\Importer.hpp>
#include <assimp\scene.h>
#include <assimp\postprocess.h>

#define STB_IMAGE_IMPLEMENTATION 1
#define STB_TRUETYPE_IMPLEMENTATION 1
#define STB_RECT_PACK_IMPLEMENTATION 1
#include <stb_image.h>
#include <stb_rect_pack.h>
#include <stb_truetype.h>

#include <iostream>
#include <fstream>
#include <filesystem>

inline void fromAssimp(const aiVector3D& from, glm::vec2* to) {
	to->x = from.x;
	to->y = from.y;
}

inline void fromAssimp(const aiVector3D& from, glm::vec3* to) {
	to->x = from.x;
	to->y = from.y;
	to->z = from.z;
}

inline void fromAssimp(const aiQuaternion& from, glm::quat* to) {
	to->w = from.w;
	to->x = from.x;
	to->y = from.y;
	to->z = from.z;
}

std::string cleanPath(std::string path) {
	std::experimental::filesystem::path cleaned = path;

	if (cleaned.is_relative() || !cleaned.has_filename())
		return "";

	return cleaned.string();
}

void compileShader(GLuint type, GLuint* shader, const std::string& filePath) {
	if (*shader == 0)
		*shader = glCreateShader(type);

	// load file to string
	std::fstream stream;

	stream.open(filePath, std::fstream::in);

	if (!stream.is_open())
		return;

	const std::string source = std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());

	stream.close();

	// compile shader source
	const GLchar* sourcePtr = (const GLchar*)(source.c_str());

	glShaderSource(*shader, 1, &sourcePtr, 0);
	glCompileShader(*shader);

	GLint compiled;
	glGetShaderiv(*shader, GL_COMPILE_STATUS, &compiled);

	// if compiled, return like everything is fine
	if (compiled == GL_TRUE)
		return;

	// print error message
	GLint length = 0;
	glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &length);
	
	std::string message;
	message.resize(length);

	glGetShaderInfoLog(*shader, length, &length, &message[0]);

	std::cerr << message << std::endl;

	// clean-up like shader was never created
	glDeleteShader(*shader);
	*shader = 0;
}

void bufferMesh(const GlLoader::AttributeInfo& attributeInfo, GlLoader::MeshContext* meshContext, const aiMesh& mesh) {
	assert(meshContext); // sanity

	// gen buffers if new meshContext
	if (!meshContext->indexCount) {
		assert(!meshContext->arrayObject && !meshContext->indexBuffer && !meshContext->vertexBuffer); // sanity

		glGenVertexArrays(1, &meshContext->arrayObject);
		glGenBuffers(1, &meshContext->vertexBuffer);
		glGenBuffers(1, &meshContext->indexBuffer);
	}

	// bind buffers
	glBindVertexArray(meshContext->arrayObject);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshContext->indexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, meshContext->vertexBuffer);

	// buffer index data
	meshContext->indexCount = mesh.mNumFaces * 3;

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshContext->indexCount * sizeof(uint32_t), nullptr, GL_STATIC_DRAW);

	for (uint32_t i = 0; i < mesh.mNumFaces; i++)
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, i * sizeof(uint32_t) * 3, sizeof(uint32_t) * 3, mesh.mFaces[i].mIndices);

	// buffer vertex data
	size_t positionsSize = 3 * mesh.mNumVertices * sizeof(float)* (uint32_t)mesh.HasPositions();
	size_t normalSize = 3 * mesh.mNumVertices * sizeof(float)* (uint32_t)mesh.HasNormals();
	size_t textureCoordsSize = 2 * mesh.mNumVertices * sizeof(float)* (uint32_t)mesh.HasTextureCoords(0);

	glBufferData(GL_ARRAY_BUFFER, positionsSize + normalSize + textureCoordsSize, 0, GL_STATIC_DRAW);

	// positions
	if (positionsSize) {
		glEnableVertexAttribArray(attributeInfo.positionAttrLoc);
		glVertexAttribPointer(attributeInfo.positionAttrLoc, 3, GL_FLOAT, GL_FALSE, 0, (void*)(0));
		glBufferSubData(GL_ARRAY_BUFFER, 0, positionsSize, mesh.mVertices);
	}

	// normals
	if (normalSize) {
		glEnableVertexAttribArray(attributeInfo.normalAttrLoc);
		glVertexAttribPointer(attributeInfo.normalAttrLoc, 3, GL_FLOAT, GL_FALSE, 0, (void*)(positionsSize));
		glBufferSubData(GL_ARRAY_BUFFER, positionsSize, normalSize, mesh.mNormals);
	}

	// texcoords (individually buffering vec3s to vec2s)
	if (textureCoordsSize) {
		glEnableVertexAttribArray(attributeInfo.texcoordAttrLoc);
		glVertexAttribPointer(attributeInfo.texcoordAttrLoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)(positionsSize + normalSize));

		for (uint32_t i = 0; i < mesh.mNumVertices; i++)
			glBufferSubData(GL_ARRAY_BUFFER, positionsSize + normalSize + (i * 2 * sizeof(float)), 2 * sizeof(float), &mesh.mTextureCoords[0][i]);
	}
}

uint32_t recusriveBufferMesh(const GlLoader::AttributeInfo& attributeInfo, const aiScene& scene, const aiNode& node, GlLoader::MeshHierarchy* meshHierarchy, uint32_t parentNode = 0, uint32_t nodeCounter = 0) {
	assert(meshHierarchy); // sanity

	uint32_t currentNode = nodeCounter;
	GlLoader::MeshHierarchy::Node& meshNode = meshHierarchy->nodes[nodeCounter];

	aiVector3D position, scale;
	aiQuaternion rotation;
	node.mTransformation.Decompose(scale, rotation, position);

	fromAssimp(position, &meshNode.position);
	fromAssimp(rotation, &meshNode.rotation);
	fromAssimp(scale, &meshNode.scale);

	meshNode.parentNodeIndex = parentNode;

	if (node.mNumMeshes) {
		meshNode.name = node.mName.C_Str();

		meshNode.hasMesh = true;
		meshNode.meshContextIndex = node.mMeshes[0];

		GlLoader::MeshContext& meshContext = meshHierarchy->meshContexts[meshNode.meshContextIndex];
		bufferMesh(attributeInfo, &meshContext, *scene.mMeshes[meshNode.meshContextIndex]);
	}
	
	for (uint32_t i = 0; i < node.mNumChildren; i++)
		nodeCounter = recusriveBufferMesh(attributeInfo, scene, *node.mChildren[i], meshHierarchy, currentNode, nodeCounter + 1);

	return nodeCounter;
}

uint32_t recursirveNodeCount(const aiNode& node, uint32_t count = 0) {
	for (uint32_t i = 0; i < node.mNumChildren; i++)
		count = recursirveNodeCount(*node.mChildren[i], count);

	return count + 1;
}

GlLoader::GlLoader(AttributeInfo attributeInfo) : _attributeInfo(attributeInfo) {
	stbi_set_flip_vertically_on_load(true);
}

const GlLoader::ProgramContext* GlLoader::loadProgram(const std::string& vertexFile, const std::string& fragmentFile, bool reload) {
	std::string vertPath = cleanPath(vertexFile);
	std::string fragPath = cleanPath(fragmentFile);

	if (vertPath == "" || fragPath == "")
		return nullptr;

	const std::string combinedName = vertPath + '|' + fragPath;
	auto programIter = _loadedPrograms.find(combinedName);

	if (!reload && programIter != _loadedPrograms.end())
		return &programIter->second;

	GLuint vertexShader = _loadedShaders[fragPath];
	GLuint fragmentShader = _loadedShaders[vertPath];

	if (!vertexShader || reload) {
		compileShader(GL_VERTEX_SHADER, &vertexShader, vertPath);
		_loadedShaders[vertPath] = vertexShader;
	}

	if (!fragmentShader || reload) {
		compileShader(GL_FRAGMENT_SHADER, &fragmentShader, fragPath);
		_loadedShaders[fragPath] = fragmentShader;
	}

	if (!vertexShader || !fragmentShader)
		return nullptr;

	GLuint program;

	if (programIter != _loadedPrograms.end())
		program = programIter->second.program;
	else
		program = glCreateProgram();

	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);

	glLinkProgram(program);

	GLint success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);

	if (success) {
		ProgramContext& programContext = _loadedPrograms[combinedName];
		programContext.program = program;

		return &programContext;
	}

	GLint length = 0;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

	std::string message;
	message.resize(length);

	glGetProgramInfoLog(program, length, &length, &message[0]);

	glDeleteProgram(program);
	program = 0;

	std::cerr << message << std::endl;
	return nullptr;
}

const GlLoader::TextureContext* GlLoader::loadTexture(const std::string& textureFile, bool reload) {
	std::string texturePath = cleanPath(textureFile);

	if (texturePath == "")
		return nullptr;

	auto textureIter = _loadedTextures.find(texturePath);

	if (!reload && textureIter != _loadedTextures.end())
		return &textureIter->second;

	int width, height, channels;
	uint8_t* data = stbi_load(texturePath.c_str(), &width, &height, &channels, 4);

	if (!data)
		return nullptr;

	TextureContext& textureContext = _loadedTextures[texturePath];

	if (textureIter == _loadedTextures.end())
		glGenTextures(1, &textureContext.textureBuffer);

	glBindTexture(GL_TEXTURE_2D, textureContext.textureBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)data);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	stbi_image_free(data);

	return &textureContext;
}

const GlLoader::MeshHierarchy* GlLoader::loadMesh(const std::string& meshFile, bool reload) {
	std::string meshPath = cleanPath(meshFile);

	if (meshPath == "")
		return nullptr;

	auto meshIter = _loadedMeshes.find(meshPath);

	if (!reload && meshIter != _loadedMeshes.end())
		return &meshIter->second;

	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(meshPath, aiProcessPreset_TargetRealtime_MaxQuality);

	if (!scene || !scene->mNumMeshes)
		return nullptr;

	MeshHierarchy& meshHierarchy = _loadedMeshes[meshPath];

	meshHierarchy.meshContexts.resize(scene->mNumMeshes);
	meshHierarchy.nodes.resize(recursirveNodeCount(*scene->mRootNode));

	recusriveBufferMesh(_attributeInfo, *scene, *scene->mRootNode, &meshHierarchy);

	return &meshHierarchy;
}
