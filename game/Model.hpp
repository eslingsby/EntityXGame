#pragma once

#include "GlLoader.hpp"
#include <string>
#include <cstdint>

class Model{
public:
	struct FilePaths {
		std::string meshFile = "";
		uint32_t meshIndex = 0;
		std::string textureFile = "";
	};

	const FilePaths filePaths;

	GlLoader::MeshContext meshContext;
	GlLoader::TextureContext textureContext;

	Model(FilePaths filePaths) : filePaths(filePaths) {}
};