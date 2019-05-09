#pragma once

struct Sound {
	const std::string soundFile;
	int sourceContextIndex = -1;

	struct Settings {
		float radius = 10000.f;
		uint32_t falloffPower = 3;

		// currently ignored
		bool playing = true;
		bool loop = true;
		bool attenuated = true;
		bool binaural = true;

		bool seeking = false;
		double seek = 0.0;
	} settings;

	Sound(const std::string& soundFile, const Settings& settings) : soundFile(soundFile), settings(settings) {};
};