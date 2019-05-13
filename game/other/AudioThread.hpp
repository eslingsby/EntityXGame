#pragma once

#include <glm\vec3.hpp>
#include <glm\gtc\quaternion.hpp>
#include <thread>
#include <mutex>

using IPLhandle = void*;

struct SoundIo;
struct SoundIoDevice;
struct SoundIoOutStream;

// callback to get input sample data for each source. user could set it to return sample data from file, stream it, or synthesize it
typedef uint32_t(*AudioInputCallback)(const void* const userData, uint32_t sampleRate, uint8_t channels, uint32_t samplesRequested, uint32_t currentSample, const float** interleavedSamples);

struct AudioInput {
	void* userData = nullptr; // user data passed to callback (i.e. ptr to sample data loaded from file)
	uint8_t channels = 0; // channels, only 1 or 2
	uint32_t sampleCount = 0; // samplecount for currentSample and looping (need to implement 0 as realtime)
	AudioInputCallback inputCallback = nullptr; // callback to pass in sample data

	// current sample iterated after callback. On end, resets to 0 if loop is enabled, otherwise stays at sampleCount.
	uint32_t currentSample = 0;
};

struct AudioSource {
	struct SoundSettings {
		bool playing = true;
		bool loop = true;

		float volume = 1.f; // volume, 0 to 1

		bool seeking = false; // set to true to seek (gets flipped back on seek)
		double seek = 0.0; // position to seek to

		bool attenuated = true; // apply distance attenuation (currently ignored)
		bool binaural = true; // apply hrtf directional effects (currently ignored)

		float radius = 1000.f; // falloff radius
		uint32_t falloffPower = 1; // exponent to apply to volume within radius (higher means sharper falloff)
	} soundSettings;

	glm::vec3 globalPosition;
	glm::quat globalRotation;
};

struct AudioListener {
	glm::vec3 globalPosition;
	glm::quat globalRotation;
};

struct AudioThreadContext {
	struct SourceContext {
		// soundioWriteCallback plays all valid sourceContexts. set to false for reuse later
		bool valid = false;
		
		AudioSource audioSource; // audio source data (position, radius, volume)
		AudioInput audioInput; // audio input data (container for ptr to raw samples from AudioInputCallback)
		
		// per source phonon objects
		IPLhandle directSoundEffect = nullptr;
		IPLhandle binauralObjectEffect = nullptr;

		// memory buffer for phonon rendering each stage
		std::vector<float> inBuffer;
		std::vector<float> middleBuffer;
		std::vector<float> outBuffer;

		// lock for reading/writing SourceContext data (i.e. initiating audioInput and phonon objects, updating audioSource) 
		//std::mutex sourceLock;
		//
		//SourceContext() {}
		//
		//SourceContext(const SourceContext& other) :
		//	valid(other.valid),
		//	audioSource(other.audioSource),
		//	audioInput(other.audioInput),
		//	directSoundEffect(other.directSoundEffect),
		//	binauralObjectEffect(other.binauralObjectEffect),
		//	inBuffer(other.inBuffer),
		//	middleBuffer(other.middleBuffer),
		//	outBuffer(other.outBuffer){
		//}
	};

	uint32_t sampleRate = 0; // samples per second
	uint32_t frameSize = 0; // max samples to write on soundioWriteCallback

	// listenenr transform info
	AudioListener listener;

	// list of sources, and a free list for re-use
	std::vector<SourceContext> sourceContexts;
	std::vector<uint32_t> freeSourceContexts;

	// memory buffer for phonon rendering final mix
	std::vector<float> mixBuffer;

	// soundio objects
	SoundIo* soundIo = nullptr;
	SoundIoDevice* soundIoDevice = nullptr;
	SoundIoOutStream* soundIoOutStream = nullptr;

	// global phonon objects
	IPLhandle phononContext = nullptr;
	IPLhandle phononBinauralRenderer = nullptr;
	IPLhandle phononEnvironment = nullptr;
	IPLhandle phononEnvironmentRenderer = nullptr;

	// thread that calls soundioWriteCallback
	std::thread audioThread;

	// lock for reading/writing AudioThreadContext data (i.e. changing size of sourceContexts)
	std::mutex threadLock;

	// if error happens during callback, soundioWriteCallback will skip
	bool error = false;
};

bool createAudioThread(AudioThreadContext* threadContext, uint32_t sampleRate, uint32_t frameSize);
void destroyAudioThread(AudioThreadContext* threadContext);

int createAudioSource(AudioThreadContext* threadContext, const AudioInput& audioInput, const AudioSource& audioSource = AudioSource());
void freeAudioSource(AudioThreadContext* threadContext, int sourceIndex);

void setAudioListener(AudioThreadContext* threadContext, const AudioListener& listener);
void setAudioSource(AudioThreadContext* threadContext, int sourceIndex, const AudioSource& audioSource);