#pragma once

#include <filesystem>

inline std::string formatPath(const std::string& globalPath, const std::string& relativePath) {
	std::experimental::filesystem::path path = globalPath;
	path.replace_filename(relativePath);

	return path.string();
}