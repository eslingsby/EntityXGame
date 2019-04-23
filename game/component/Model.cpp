#include "component\Model.hpp"

Model::Model(FilePaths filePaths) : filePaths(filePaths) {}

void Model::setModel(entityx::Entity self, const FilePaths& filePaths) {
	self.remove<Model>();
	self.assign<Model>(filePaths);
}

void Model::setMesh(entityx::Entity self, const std::string& meshFile, uint32_t meshIndex) {
	const FilePaths newFilePaths = FilePaths{ meshFile, meshIndex, filePaths.textureFile };

	self.remove<Model>();
	self.assign<Model>(newFilePaths);
}

void Model::setTexture(entityx::Entity self, const std::string& textureFile) {
	const FilePaths newFilePaths = FilePaths{ filePaths.meshFile, filePaths.meshIndex, textureFile };

	self.remove<Model>();
	self.assign<Model>(newFilePaths);
}