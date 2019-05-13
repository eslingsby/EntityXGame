#pragma once

#include <entityx\System.h>

#include "system\PhysicsEvents.hpp"

#include "component\Transform.hpp"
#include "component\Listener.hpp"
#include "component\Sound.hpp"

#include "other\AudioThread.hpp"

#include <unordered_map>

#include <libnyquist\Decoders.h>

class Audio : public entityx::System<Audio>, public entityx::Receiver<Audio> {
	const std::string _path;
	const uint32_t _sampleRate;
	const uint32_t _frameSize;

	AudioThreadContext _audioThread;

	nqr::NyquistIO _audioLoader;
	std::unordered_map<std::string, nqr::AudioData> _loadedSounds;

	entityx::Entity _listenerEntity;

	nqr::AudioData* _loadAudio(const std::string& file);

	void _updateListener();
	void _updateSource(entityx::Entity sourceEntity);
	
public:
	struct ConstructorInfo {
		std::string path = "";
		uint32_t sampleRate = 48000;
		uint32_t frameSize = 512; // 512 min
	};

	Audio(const ConstructorInfo& constructorInfo);
	~Audio();

	void configure(entityx::EventManager &events) final;
	void update(entityx::EntityManager &entities, entityx::EventManager &events, double dt) final;

	void receive(const entityx::ComponentAddedEvent<Listener>& listenerAddedEvent);
	void receive(const entityx::ComponentAddedEvent<Sound>& soundAddedEvent);
	void receive(const entityx::ComponentRemovedEvent<Sound>& soundAddedEvent);
	void receive(const entityx::ComponentAddedEvent<Transform>& transformAddedEvent);
	void receive(const entityx::ComponentRemovedEvent<Transform>& transformAddedEvent);
	void receive(const CollidingEvent& collidingEvent);
	void receive(const ContactEvent& contactEvent);
};