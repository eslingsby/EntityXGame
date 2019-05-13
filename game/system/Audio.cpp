#include "system\Audio.hpp"

#include "component\Name.hpp"
#include "component\Collider.hpp"

#include "other\Path.hpp"

#include <libnyquist\Decoders.h>

#include <iostream>

nqr::AudioData * Audio::_loadAudio(const std::string & file){
	std::string filePath = formatPath(_path, file);

	nqr::AudioData* audioData;

	auto i = _loadedSounds.find(filePath);

	if (i == _loadedSounds.end()) {
		audioData = &_loadedSounds[filePath];

		_audioLoader.Load(audioData, filePath);

		if (!audioData->samples.size()) {
			std::cerr << "Audio NyquistIO: couldn't load " << filePath << std::endl;
			_loadedSounds.erase(filePath);
			return nullptr;
		}
	}
	else {
		audioData = &i->second;
	}

	return audioData;
}

void Audio::_updateListener(){
	AudioListener listener;
	_listenerEntity.component<const Transform>()->globalDecomposed(&listener.globalPosition, &listener.globalRotation);

	setAudioListener(&_audioThread, listener);
}

void Audio::_updateSource(entityx::Entity sourceEntity){
	auto transform = sourceEntity.component<Transform>();
	auto sound = sourceEntity.component<Sound>();

	// Skip if sound has no source context
	if (sound->sourceContextIndex < 0)
		return;

	// update source
	AudioSource audioSource;
	audioSource.soundSettings = sound->settings;

	transform->globalDecomposed(&audioSource.globalPosition, &audioSource.globalRotation);

	setAudioSource(&_audioThread, sound->sourceContextIndex, audioSource);
}

Audio::Audio(const ConstructorInfo& constructorInfo) :
		_sampleRate(constructorInfo.sampleRate), 
		_frameSize(constructorInfo.frameSize),
		_path(constructorInfo.path) {

	createAudioThread(&_audioThread, 48000, 512);
}

Audio::~Audio(){
	destroyAudioThread(&_audioThread);
}

void Audio::configure(entityx::EventManager & events){
	events.subscribe<ContactEvent>(*this);
	events.subscribe<CollidingEvent>(*this);
	events.subscribe<entityx::ComponentAddedEvent<Listener>>(*this);
	events.subscribe<entityx::ComponentAddedEvent<Sound>>(*this);
	events.subscribe<entityx::ComponentRemovedEvent<Sound>>(*this);
	events.subscribe<entityx::ComponentAddedEvent<Transform>>(*this);
	events.subscribe<entityx::ComponentRemovedEvent<Transform>>(*this);
}

void Audio::update(entityx::EntityManager & entities, entityx::EventManager & events, double dt){
	if (!_listenerEntity.valid() || !_listenerEntity.has_component<Transform>() || !_listenerEntity.has_component<Listener>())
		return;

	// update listener
	_updateListener();

	// update sources
	for (auto entity : entities.entities_with_components<Transform, Sound>())
		_updateSource(entity);
}

uint32_t audioInputCallback(const void* const userData, uint32_t sampleRate, uint8_t channels, uint32_t samplesRequested, uint32_t currentSample, const float** interleavedSamples) {
	const nqr::AudioData* audioData = (nqr::AudioData*)userData;
	
	*interleavedSamples = &audioData->samples[currentSample * channels];
	
	return 0; // not used atm (maybe remove?)
}

void Audio::receive(const entityx::ComponentAddedEvent<Listener>& listenerAddedEvent){
	_listenerEntity = listenerAddedEvent.entity;

	if (_listenerEntity.has_component<Transform>())
		_updateListener();
}

void Audio::receive(const entityx::ComponentAddedEvent<Sound>& soundAddedEvent) {
	auto sound = soundAddedEvent.component;

	// Load audio data
	nqr::AudioData* audioData = _loadAudio(sound->soundFile);

	// Setup audio input and callback
	AudioInput audioInput;
	audioInput.userData = audioData;
	audioInput.channels = audioData->channelCount;
	audioInput.sampleCount = audioData->samples.size() / audioInput.channels;
	audioInput.inputCallback = audioInputCallback;

	// Setup initial audio source
	AudioSource audioSource;

	// If transform, set position, else not playing
	if (soundAddedEvent.entity.has_component<Transform>()) {
		auto transform = soundAddedEvent.entity.component<const Transform>();
		transform->globalDecomposed(&audioSource.globalPosition, &audioSource.globalRotation);
	}
	else {
		sound->settings.playing = false; // don't play for now
	}

	audioSource.soundSettings = sound->settings;

	// Create audio source
	sound->sourceContextIndex = createAudioSource(&_audioThread, audioInput, audioSource);
}

void Audio::receive(const entityx::ComponentRemovedEvent<Sound>& soundAddedEvent){
	auto sound = soundAddedEvent.component;

	if (sound->sourceContextIndex != -1)
		freeAudioSource(&_audioThread, sound->sourceContextIndex);
}

void Audio::receive(const entityx::ComponentAddedEvent<Transform>& transformAddedEvent) {
	if (!transformAddedEvent.entity.has_component<Sound>())
		return;

	entityx::Entity soundEntity = transformAddedEvent.entity;

	auto sound = soundEntity.component<Sound>();
	sound->settings.playing = true;

	_updateSource(transformAddedEvent.entity);
}

void Audio::receive(const entityx::ComponentRemovedEvent<Transform>& transformAddedEvent) {
	if (!transformAddedEvent.entity.has_component<Sound>())
		return;

	entityx::Entity soundEntity = transformAddedEvent.entity;

	auto sound = soundEntity.component<Sound>();
	sound->settings.playing = false;

	_updateSource(transformAddedEvent.entity);
}

//void entityCollidingEvent(entityx::Entity entity) {
//	if (!entity.has_component<Sound>())
//		return;
//
//	auto sound = entity.component<Sound>();
//
//	//if (!sound->settings.physical)
//	//	return;
//
//	sound->settings.playing = true;
//	sound->settings.seeking = true;
//	sound->settings.seek = 0;
//}

void Audio::receive(const CollidingEvent & collidingEvent){
	//entityCollidingEvent(collidingEvent.firstEntity);
	//entityCollidingEvent(collidingEvent.secondEntity);
}

//void entityContactEvent(entityx::Entity entity, const ContactEvent & contactEvent) {
//	if (!entity.has_component<Sound>())
//		return;
//
//	float impulse = 0.f;
//	float distance = 0.f;
//
//	for (uint8_t i = 0; i < contactEvent.contactCount; i++) {
//		impulse += contactEvent.contacts[i].contactImpulse;
//		distance += contactEvent.contacts[i].contactDistance;
//	}
//
//	auto collider = entity.component<Collider>();
//	float relativeImpulse = impulse * collider->getInvMass();
//
//	if (relativeImpulse < 50)
//		return;
//
//	auto sound = entity.component<Sound>();
//	sound->settings.falloffPower = glm::clamp(relativeImpulse, 0.f, 1000.f) / 1000.f;
//}

void Audio::receive(const ContactEvent & contactEvent){
	//entityContactEvent(contactEvent.firstEntity, contactEvent);
	//entityContactEvent(contactEvent.secondEntity, contactEvent);
}