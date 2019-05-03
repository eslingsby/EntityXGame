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

	uint32_t _sampleRate = 44100;

	std::thread _thread;

	entityx::Entity _listenerEntity;
	entityx::Entity _soundEntity; // testing, just the one for now

	struct SourceContext {
		IPLhandle binauralObjectEffect = nullptr;
		IPLSource source;

		IPLfloat32 radius;

		IPLDirectSoundEffectOptions effectOptions;
		IPLDirectSoundPath soundPath;

		IPLAudioBuffer inBufferContext; // raw samples
		IPLAudioBuffer middleBufferContext; // binarual samples
		IPLAudioBuffer outBufferContext; // direct sound samples

		// replace with arrays, or pointers to shared block of memory
		std::vector<float> inBuffer;
		std::vector<float> middleBuffer;
		std::vector<float> outBuffer;
	};

public:
	struct ThreadContext {
		IPLhandle phononEnvironment = nullptr;
		IPLhandle phononDirectSoundEffect = nullptr;
		IPLhandle phononBinauralRenderer = nullptr;

		uint32_t frameSize = 2048;

		std::mutex positionMutex;

		glm::vec3 listenerGlobalPosition;
		glm::quat listenerGlobalRotation;

		glm::vec3 sourceGlobalPosition; // testing, just the one for now
		glm::quat sourceGlobalRotation; // testing, just the one for now
		
		SourceContext sourceContext; // testing, just the one for now 
	};
	
private:
	ThreadContext _threadContext;

public:
	Audio();
	~Audio();

	void configure(entityx::EventManager &events) final;
	void update(entityx::EntityManager &entities, entityx::EventManager &events, double dt) final;

	void receive(const entityx::ComponentAddedEvent<Listener>& listenerAddedEvent);
	void receive(const entityx::ComponentAddedEvent<Sound>& soundAddedEvent);
	void receive(const CollidingEvent& collidingEvent);
	void receive(const ContactEvent& contactEvent);
};