#pragma once

struct Sound {
	const std::string soundFile;
	int sourceContextIndex = -1;

	struct Settings {
		float radius = 10000.f;
		uint32_t falloffPower = 3;

		bool physical = false;

		bool loop = true;

		bool seeking = false;
		double seek = 0;

		float power = 1;

		// currently ignored
		bool playing = true;
		bool attenuated = true;
		bool binaural = true;
	} settings;

	Sound(const std::string& soundFile, const Settings& settings) : soundFile(soundFile), settings(settings) {};
};