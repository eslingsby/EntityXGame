#pragma once

#include "other\AudioThread.hpp"

struct Sound {
	using Settings = AudioSource::SoundSettings;

	const std::string soundFile;
	int sourceContextIndex = -1;

	Settings settings;

	Sound(const std::string& soundFile, const AudioSource::SoundSettings& settings) : 
		soundFile(soundFile),
		settings(settings) {
	};
};