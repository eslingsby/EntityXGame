#pragma once

#include <glad\glad.h>

#include <string>
#include <vector>
#include <unordered_map>

#include <glm\vec3.hpp>
#include <glm\gtc\quaternion.hpp>

class GlLoader {
public:
	struct MeshContext {
		GLuint arrayObject = 0;
		GLuint vertexBuffer = 0;
		GLuint indexBuffer = 0;
		uint32_t indexCount = 0;
	};

	struct TextureContext {
		GLuint textureBuffer = 0;
	};

	struct ProgramContext {
		GLuint program = 0;
	};

	struct MeshHierarchy {
		struct Node {
			bool hasMesh = false;

			uint32_t parentNodeIndex = 0;
			uint32_t meshContextIndex = 0;

			glm::vec3 position;
			glm::quat rotation;
			glm::vec3 scale = { 1, 1, 1 };

			std::string name;
		};

		std::vector<MeshContext> meshContexts;
		std::vector<Node> nodes;
	};
	
	struct AttributeInfo {
		uint32_t positionAttrLoc = 0;
		uint32_t normalAttrLoc = 1;
		uint32_t texcoordAttrLoc = 2;
		//uint32_t tangentAttrLoc = 3;
		//uint32_t bitangentAttrLoc = 4;
		//uint32_t colourAttrLoc = 5;
		//uint32_t boneWeightsAttrLoc = 6;
		//uint32_t boneIndexesAttrLoc = 7;
	};

	struct MeshData {
		const glm::vec3* positions = nullptr;
		uint32_t positionCount = 0;

		const glm::vec2* texcoords = nullptr;
		uint32_t texcoordCount = 0;

		const glm::vec3* normals = nullptr;
		uint32_t normalCount = 0;
	};

	struct TextureData {
		const glm::tvec4<uint8_t>* colours = nullptr;
		glm::uvec2 size;
	};

private:
	const AttributeInfo _attributeInfo;

	std::unordered_map<std::string, GLuint> _loadedShaders;
	std::unordered_map<std::string, ProgramContext> _loadedPrograms;
	std::unordered_map<std::string, TextureContext> _loadedTextures;
	std::unordered_map<std::string, MeshHierarchy> _loadedMeshes;

public:
	GlLoader(AttributeInfo attributeInfo);

	const ProgramContext* loadProgram(const std::string& vertexFile, const std::string& fragmentFile, bool reload = false);
	const TextureContext* loadTexture(const std::string& textureFile, bool reload = false);
	const MeshHierarchy* loadMesh(const std::string& meshFile, bool reload = false);

	void getTextures(std::vector<TextureContext>* textureContexts) const;
	void getMeshes(std::vector<MeshContext>* meshContexts) const;

	void mapMesh(const MeshContext& meshContext, MeshData* meshData);
	void unmapMesh(const MeshContext& meshContext);

	void mapTexture(const TextureContext& textureContext, TextureData* textureData);
	void unmapTexture(const TextureContext& textureContext);
};