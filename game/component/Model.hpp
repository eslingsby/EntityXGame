#pragma once

#include "other\GlLoader.hpp"

#include <string>
#include <cstdint>

#include <entityx\Entity.h>

struct Model{
	struct FilePaths {
		std::string meshFile = "";
		uint32_t meshIndex = 0;
		std::string textureFile = "";
	};

	const FilePaths filePaths;

	GlLoader::MeshContext meshContext;
	GlLoader::TextureContext textureContext;

	Model(FilePaths filePaths);

	void setModel(entityx::Entity self, const FilePaths& filePaths);
	void setMesh(entityx::Entity self, const std::string& meshFile, uint32_t meshIndex = 0);
	void setTexture(entityx::Entity self, const std::string& textureFile);
};