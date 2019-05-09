#pragma once

#include <entityx\System.h>

#include "system\PhysicsEvents.hpp"

#include "component\Listener.hpp"
#include "component\Sound.hpp"

#include <vector>
#include <thread>
#include <mutex>

#include <glm\gtc\quaternion.hpp>

#include <phonon.h>
#include <libnyquist\Decoders.h>

struct SoundIo;
struct SoundIoDevice;
struct SoundIoOutStream;

class Audio : public entityx::System<Audio>, public entityx::Receiver<Audio> {
	SoundIo* _soundIo = nullptr;;
	SoundIoDevice* _soundIoDevice = nullptr;;
	SoundIoOutStream* _soundIoOutStream = nullptr;;

	IPLhandle _phononContext = nullptr;
	IPLhandle _phononEnvironmentRenderer = nullptr;

	IPLAudioFormat _phononMono;
	IPLAudioFormat _phononStereo;

	const uint32_t _sampleRate;
	const uint32_t _frameSize;

	const std::string _path;

	std::thread _thread;

	entityx::Entity _listenerEntity;

	nqr::NyquistIO _audioLoader;

	std::unordered_map<std::string, nqr::AudioData> _loadedSounds;
	
	struct SourceContext {
		bool active = true;

		IPLhandle directSoundEffect = nullptr;
		IPLhandle binauralObjectEffect = nullptr;

		const nqr::AudioData* audioData;
		uint32_t currentSample = 0;

		glm::vec3 globalPosition;
		glm::quat globalRotation;

		Sound::Settings soundSettings;

		IPLAudioBuffer inBufferContext; // raw samples
		IPLAudioBuffer middleBufferContext; // binarual samples
		IPLAudioBuffer outBufferContext; // direct sound samples

		// replace with arrays, or pointers to shared block of memory
		std::vector<float> inBuffer;
		std::vector<float> middleBuffer;
		std::vector<float> outBuffer;
	};

	struct ThreadContext {
		uint32_t sampleRate;
		uint32_t frameSize;

		IPLhandle phononEnvironment = nullptr;
		IPLhandle phononBinauralRenderer = nullptr;

		IPLDirectSoundEffectOptions effectOptions;

		std::mutex mutex;

		glm::vec3 listenerGlobalPosition;
		glm::quat listenerGlobalRotation;

		std::vector<SourceContext> sourceContexts;
		std::vector<uint32_t> freeSourceContexts;
	
		std::vector<IPLAudioBuffer> unmixedBuffers;

		std::vector<float> mixBuffer;
		IPLAudioBuffer mixBufferContext;
	} _threadContext;

public:
	struct ConstructorInfo {
		std::string path = "";
		uint32_t sampleRate = 44100;
		uint32_t frameSize = 1024; // 512 min
	};

public:
	Audio(const ConstructorInfo& constructorInfo);
	~Audio();

	void configure(entityx::EventManager &events) final;
	void update(entityx::EntityManager &entities, entityx::EventManager &events, double dt) final;

	void receive(const entityx::ComponentAddedEvent<Listener>& listenerAddedEvent);
	void receive(const entityx::ComponentAddedEvent<Sound>& soundAddedEvent);
	void receive(const entityx::ComponentRemovedEvent<Sound>& soundAddedEvent);
	void receive(const CollidingEvent& collidingEvent);
	void receive(const ContactEvent& contactEvent);

	friend void writeCallback(SoundIoOutStream* outstream, int frameCountMin, int frameCountMax);
};