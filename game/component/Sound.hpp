#pragma once

struct Sound {
	const std::string soundFile;
	uint32_t sourceContextIndex;

	Sound(const std::string& soundFile) : soundFile(soundFile) {};
};