#include "system\Audio.hpp"

#include "component\Name.hpp"
#include "component\Transform.hpp"

#include "other\Path.hpp"

#include <iostream>

#include <soundio\soundio.h>

// Copied from phonon documentation: positive x-axis pointing right, positive y-axis pointing up, and the negative z-axis pointing ahead.
IPLVector3 toPhonon(const glm::vec3& vector) {
	return { vector.x, vector.z, -vector.y };
}

void phononLog(char* message) {
	std::cerr << "Audio phonon: " << message << std::endl;
}

const char* phononErrorMsg(IPLerror error) {
	// Copied over from phonon comments:
	switch (error) {
		case IPL_STATUS_SUCCESS: return "operation completed successfully";
		case IPL_STATUS_FAILURE: return "unspecified error occurred";
		case IPL_STATUS_OUTOFMEMORY: return "system ran out of memory";
		case IPL_STATUS_INITIALIZATION: return "error occurred while initializing an external dependency";
	}

	return "";
}

void writeCallback(SoundIoOutStream* outstream, int frameCountMin, int frameCountMax) {
	Audio::ThreadContext& threadContext = *(Audio::ThreadContext*)outstream->userdata;

	SoundIoChannelArea* areas;
	int framesLeft = threadContext.frameSize;
	int soundioError;

	// Doing source context stuff, so lock them
	std::unique_lock<std::mutex> lock(threadContext.mutex);

	if (!threadContext.sourceContexts.size())
		return;

	while (framesLeft > 0) {
		// open outstream
		int frameCount = framesLeft;

		if (soundioError = soundio_outstream_begin_write(outstream, &areas, &frameCount)) {
			std::cerr << "Audio soundio_outstream_begin_write: " << soundio_strerror(soundioError) << std::endl;
			return;
		}

		if (frameCount <= 0)
			break;

		threadContext.unmixedBuffers.clear();
	
		for (Audio::SourceContext& sourceContext : threadContext.sourceContexts) {
			if (!sourceContext.active)
				continue;
	
			uint32_t channels = sourceContext.audioData->channelCount;
			uint32_t samplesAvailable = frameCount * channels;
			uint32_t audioSampleCount = sourceContext.audioData->samples.size();
			uint32_t sampleRate = sourceContext.audioData->sampleRate; // when audio is resampled to be the same, use threadContext.sampleRate instead

			if (sourceContext.soundSettings.seeking) {
				double length = sampleRate / audioSampleCount;
				
				if (sourceContext.soundSettings.seek <= 0)
					sourceContext.currentSample = 0;
				else if (sourceContext.soundSettings.seek >= length)
					sourceContext.currentSample = audioSampleCount;
				else
					sourceContext.currentSample = sourceContext.soundSettings.seek * sampleRate * channels;

				sourceContext.soundSettings.seeking = false;
			}

			// Non looping file that's at the end
			if (sourceContext.currentSample == audioSampleCount)
				continue;

			uint32_t samplesToFill = samplesAvailable;
			
			while (samplesToFill) {
				// get amount of samples, or whatever is leftover
				uint32_t samplesGot = glm::min(audioSampleCount - sourceContext.currentSample, samplesToFill);
			
				for (uint32_t i = 0; i < samplesGot; i++)
					sourceContext.inBuffer[samplesAvailable - samplesToFill + i] = sourceContext.audioData->samples[sourceContext.currentSample + i];
			
				samplesToFill -= samplesGot;
				sourceContext.currentSample += samplesGot;
			
				// reached end of audio data
				if (samplesGot <= samplesToFill) {
					if (!sourceContext.soundSettings.loop)
						break;
			
					sourceContext.currentSample = 0;
				}
			}

			// Tell phonon how many samples soundio wants
			threadContext.mixBufferContext.numSamples = frameCount;
			sourceContext.inBufferContext.numSamples = frameCount;
			sourceContext.middleBufferContext.numSamples = frameCount;
			sourceContext.outBufferContext.numSamples = frameCount;

			// Calculate direct path (seems broken until environment data is given to phonon???)
			//IPLDirectivity directivity;
			//directivity.dipoleWeight = 0;
			//directivity.dipolePower = 0;
			
			//IPLSource source;
			//source.directivity = directivity;
			//source.position = toPhonon(sourceContext.globalPosition); // game units are cm, steam audio is in meters
			//source.ahead = toPhonon(sourceContext.globalRotation * Transform::forward);
			//source.up = toPhonon(sourceContext.globalRotation * Transform::up);
			
			//IPLVector3 listenerGlobalPosition = toPhonon(threadContext.listenerGlobalPosition); // cm to meters
			//IPLVector3 listenerGlobalPositionForward = toPhonon(threadContext.listenerGlobalRotation * Transform::forward);
			//IPLVector3 listenerGlobalPositionUp = toPhonon(threadContext.listenerGlobalRotation * Transform::up);
			
			//IPLDirectSoundPath soundPath = iplGetDirectSoundPath(threadContext.phononEnvironment, listenerGlobalPosition, listenerGlobalPositionForward, listenerGlobalPositionUp, source, sourceContext.radius, IPL_DIRECTOCCLUSION_NONE, IPL_DIRECTOCCLUSION_RAYCAST);

			// Apply distance attenutation
			IPLDirectSoundPath soundPath; // create blank one instead (testing)

			float distance = glm::length(threadContext.listenerGlobalPosition - sourceContext.globalPosition);

			soundPath.distanceAttenuation = glm::clamp(1.f - (distance * 1.f / sourceContext.soundSettings.radius), 0.f, 1.f);

			// If outside radius, skip applying and mixing (causes click noises on enter radius???)
			if (soundPath.distanceAttenuation == 0.f)
				continue;

			soundPath.distanceAttenuation = glm::pow(soundPath.distanceAttenuation, sourceContext.soundSettings.falloffPower);

			// Apply direct sound path
			iplApplyDirectSoundEffect(sourceContext.directSoundEffect, sourceContext.inBufferContext, soundPath, threadContext.effectOptions, sourceContext.middleBufferContext);

			// Apply binaural
			IPLVector3 direction = toPhonon(glm::inverse(threadContext.listenerGlobalRotation) * glm::normalize(sourceContext.globalPosition - threadContext.listenerGlobalPosition));

			iplApplyBinauralEffect(sourceContext.binauralObjectEffect, threadContext.phononBinauralRenderer, sourceContext.middleBufferContext, direction, IPL_HRTFINTERPOLATION_BILINEAR, sourceContext.outBufferContext);
		
			threadContext.unmixedBuffers.push_back(sourceContext.outBufferContext);
		}

		// If buffers to mix, mix with phonon
		if (threadContext.unmixedBuffers.size())
			iplMixAudioBuffers(threadContext.unmixedBuffers.size(), &threadContext.unmixedBuffers[0], threadContext.mixBufferContext);

		// If mixed buffers, copy over buffer to outstream. Else copy over 0.f
		for (int frame = 0; frame < frameCount; frame += 1) {
			for (int channel = 0; channel < outstream->layout.channel_count; channel += 1) {
				float *ptr = (float*)(areas[channel].ptr + areas[channel].step * frame);

				if (threadContext.unmixedBuffers.size())
					*ptr = threadContext.mixBuffer[frame * 2 + channel];
				else
					*ptr = 0.f;
			}
		}

		// close outstream
		if (soundioError = soundio_outstream_end_write(outstream)) {
			std::cerr << "Audio soundio_outstream_end_write: " << soundio_strerror(soundioError) << std::endl;
			return;
		}

		framesLeft -= frameCount;
	}
}

Audio::Audio(const ConstructorInfo& constructorInfo) : _sampleRate(constructorInfo.sampleRate), _frameSize(constructorInfo.frameSize), _path(constructorInfo.path) {
	// Create phonon context
	IPLerror phononError = iplCreateContext(&phononLog, nullptr, nullptr, &_phononContext); // context

	if (phononError != IPL_STATUS_SUCCESS) {
		std::cerr << "Audio iplCreateContext: " << phononErrorMsg(phononError) << std::endl;
		return;
	}

	// shared variables
	_threadContext.frameSize = _frameSize;
	_threadContext.sampleRate = _sampleRate;

	_phononMono.channelLayoutType = IPL_CHANNELLAYOUTTYPE_SPEAKERS;
	_phononMono.channelLayout = IPL_CHANNELLAYOUT_MONO;
	_phononMono.channelOrder = IPL_CHANNELORDER_INTERLEAVED;

	_phononStereo = _phononMono;
	_phononStereo.channelLayout = IPL_CHANNELLAYOUT_STEREO;

	_threadContext.mixBuffer.resize(_frameSize * 2);
	_threadContext.mixBufferContext.numSamples = _frameSize;
	_threadContext.mixBufferContext.interleavedBuffer = &_threadContext.mixBuffer[0];
	_threadContext.mixBufferContext.format = _phononStereo;

	IPLRenderingSettings settings{ _sampleRate, _threadContext.frameSize, IPL_CONVOLUTIONTYPE_PHONON };

	// Create binaural renderer
	IPLHrtfParams hrtfParams{ IPL_HRTFDATABASETYPE_DEFAULT, nullptr, nullptr };
	phononError = iplCreateBinauralRenderer(_phononContext, settings, hrtfParams, &_threadContext.phononBinauralRenderer); // binaural renderer

	if (phononError != IPL_STATUS_SUCCESS) {
		std::cerr << "Audio iplCreateBinauralRenderer: " << phononErrorMsg(phononError) << std::endl;
		return;
	}

	// Create environment (comments copied over from phonon)
	IPLSimulationSettings simulationSettings;
	simulationSettings.ambisonicsOrder = 0; // increases the amount of directional detail; must be between 0 and 3
	simulationSettings.irDuration = 0; // delay between a sound being emitted and the last audible reflection; 0.5 to 4.0
	simulationSettings.maxConvolutionSources = 0; // allows more sound sources to be rendered with sound propagation effects, at the cost of increased memory consumption
	simulationSettings.bakingBatchSize = 0; // only used if sceneType is set to IPL_SCENETYPE_RADEONRAYS
	simulationSettings.irradianceMinDistance = 0; // Increasing this number reduces the loudness of reflections when standing close to a wall; decreasing this number results in a more physically realistic model
	simulationSettings.numBounces = 0; // 1 to 32
	simulationSettings.numDiffuseSamples = 0; // 32 to 4096
	simulationSettings.numOcclusionSamples = 0; // 32 to 512
	simulationSettings.numRays = 0; // 1024 to 131072

	phononError = iplCreateEnvironment(_phononContext, nullptr, simulationSettings, nullptr, nullptr, &_threadContext.phononEnvironment); // environment

	if (phononError != IPL_STATUS_SUCCESS) {
		std::cerr << "Audio iplCreateEnvironment: " << phononErrorMsg(phononError) << std::endl;
		return;
	}

	// Create environment renderer
	phononError = iplCreateEnvironmentalRenderer(_phononContext, _threadContext.phononEnvironment, settings, _phononStereo, nullptr, nullptr, &_phononEnvironmentRenderer);

	if (phononError != IPL_STATUS_SUCCESS) {
		std::cerr << "Audio iplCreateEnvironmentalRenderer: " << phononErrorMsg(phononError) << std::endl;
		return;
	}

	// Create soundio context
	_soundIo = soundio_create();

	if (!_soundIo)
		return;

	int soundioError;

	if (soundioError = soundio_connect(_soundIo)) {
		std::cerr << "Audio soundio_connect: " << soundio_strerror(soundioError) << std::endl;
		return;
	}

	soundio_flush_events(_soundIo);

	// Setup soundio device
	int defaultDeviceIndex = soundio_default_output_device_index(_soundIo);

	if (defaultDeviceIndex == -1) {
		std::cerr << "Audio soundio_default_output_device_index: no audio output devices" << std::endl;
		return;
	}

	_soundIoDevice = soundio_get_output_device(_soundIo, defaultDeviceIndex);

	if (!_soundIoDevice) {
		std::cerr << "Audio soundio_get_output_device: couldn't get audio output device" << std::endl;
		return;
	}

	std::cout << "Audio soundio_get_output_device: " << _soundIoDevice->name << std::endl;

	_threadContext.effectOptions.applyAirAbsorption = IPL_FALSE;
	_threadContext.effectOptions.applyDirectivity = IPL_FALSE;
	_threadContext.effectOptions.applyDistanceAttenuation = IPL_TRUE;
	_threadContext.effectOptions.directOcclusionMode = IPL_DIRECTOCCLUSION_NONE;

	// Create soundio out stream
	_soundIoOutStream = soundio_outstream_create(_soundIoDevice);
	_soundIoOutStream->format = SoundIoFormatFloat32NE;
	_soundIoOutStream->write_callback = writeCallback;
	_soundIoOutStream->sample_rate = _sampleRate;
	_soundIoOutStream->userdata = &_threadContext; // set userdata to threadContext to retrieve inside thread

	if (soundioError = soundio_outstream_open(_soundIoOutStream)) {
		std::cerr << "Audio soundio_outstream_open: " << soundio_strerror(soundioError) << std::endl;
		return;
	}

	if (_soundIoOutStream->layout_error) // non-fatal according to example???
		std::cerr << "Audio soundio layout_error: " << soundio_strerror(_soundIoOutStream->layout_error) << std::endl;

	if (soundioError = soundio_outstream_start(_soundIoOutStream)) {
		std::cerr << "Audio soundio_outstream_start: " << soundio_strerror(soundioError) << std::endl;
		return;
	}

	// Start soundio audio callback
	_thread = std::thread(soundio_wait_events, _soundIo);
}

Audio::~Audio(){
	// join callback thread
	soundio_wakeup(_soundIo);
	_thread.join();

	// cleanup soundio
	soundio_outstream_destroy(_soundIoOutStream);
	soundio_device_unref(_soundIoDevice);
	soundio_destroy(_soundIo);

	// cleanup steam audio
	iplDestroyEnvironmentalRenderer(&_phononEnvironmentRenderer);
	iplDestroyEnvironment(&_threadContext.phononEnvironment);
	iplDestroyBinauralRenderer(&_threadContext.phononBinauralRenderer);
	iplDestroyContext(&_phononContext);
}

void Audio::configure(entityx::EventManager & events){
	events.subscribe<ContactEvent>(*this);
	events.subscribe<CollidingEvent>(*this);
	events.subscribe<entityx::ComponentAddedEvent<Listener>>(*this);
	events.subscribe<entityx::ComponentAddedEvent<Sound>>(*this);
	events.subscribe<entityx::ComponentRemovedEvent<Sound>>(*this);
}

void Audio::update(entityx::EntityManager & entities, entityx::EventManager & events, double dt){
	if (!_listenerEntity.valid() || !_listenerEntity.has_component<Transform>() || !_listenerEntity.has_component<Listener>())
		return;

	// Doing source context stuff, so lock
	std::unique_lock<std::mutex> lock(_threadContext.mutex);

	auto listenerTransform = _listenerEntity.component<Transform>();
	listenerTransform->globalDecomposed(&_threadContext.listenerGlobalPosition, &_threadContext.listenerGlobalRotation);

	for (auto entity : entities.entities_with_components<Transform, Sound>()) {
		auto transform = entity.component<Transform>();
		auto sound = entity.component<Sound>();

		// Skip if sound has no source context
		if (sound->sourceContextIndex == -1)
			continue;

		// Update source context
		SourceContext& sourceContext = _threadContext.sourceContexts[sound->sourceContextIndex];

		transform->globalDecomposed(&sourceContext.globalPosition, &sourceContext.globalRotation);

		sourceContext.soundSettings = sound->settings;

		if (sound->settings.seeking)
			sound->settings.seeking = false;
	}
}

void Audio::receive(const entityx::ComponentAddedEvent<Listener>& listenerAddedEvent){
	_listenerEntity = listenerAddedEvent.entity;
}

void Audio::receive(const entityx::ComponentAddedEvent<Sound>& soundAddedEvent) {
	auto sound = soundAddedEvent.component;

	// Check if sound already loaded, otherwise load
	std::string filePath = formatPath(_path, sound->soundFile);

	nqr::AudioData* audioData;

	auto i = _loadedSounds.find(filePath);

	if (i == _loadedSounds.end()) {
		audioData = &_loadedSounds[filePath];

		_audioLoader.Load(audioData, filePath);

		if (!audioData->samples.size()) {
			std::cerr << "Audio NyquistIO: couldn't load " << filePath << std::endl;
			_loadedSounds.erase(filePath);
			return;
		}
	}
	else {
		audioData = &i->second;
	}

	// Check if mono or stereo
	if (!audioData->channelCount || audioData->channelCount > 2) {
		std::cerr << "Audio: 0 or too many channels " << filePath << std::endl;
		_loadedSounds.erase(filePath);
		return;
	}

	IPLAudioFormat audioFormat = (audioData->channelCount == 1 ? _phononMono : _phononStereo);

	// Doing source context stuff, so lock
	std::unique_lock<std::mutex> lock(_threadContext.mutex);

	// Create or reuse source context
	if (_threadContext.freeSourceContexts.size()) {
		sound->sourceContextIndex = *_threadContext.freeSourceContexts.rbegin();
		_threadContext.freeSourceContexts.pop_back();
	}
	else {
		sound->sourceContextIndex = _threadContext.sourceContexts.size();
		_threadContext.sourceContexts.resize(sound->sourceContextIndex + 1);
	}

	SourceContext& sourceContext = _threadContext.sourceContexts[sound->sourceContextIndex];

	// create direct sound effect and binaurel effect objects
	IPLerror phononError = iplCreateDirectSoundEffect(_phononEnvironmentRenderer, audioFormat, _phononStereo, &sourceContext.directSoundEffect); // direct sound effect

	if (phononError != IPL_STATUS_SUCCESS) {
		std::cerr << "Audio iplCreateDirectSoundEffect: " << phononErrorMsg(phononError) << std::endl;
		return;
	}

	phononError = iplCreateBinauralEffect(_threadContext.phononBinauralRenderer, _phononStereo, _phononStereo, &sourceContext.binauralObjectEffect);

	if (phononError != IPL_STATUS_SUCCESS) {
		std::cerr << "Audio iplCreateBinauralEffect: " << phononErrorMsg(phononError) << std::endl;
		return;
	}
	
	// Set buffers according to channels in audioDecoder
	sourceContext.inBufferContext.format = audioFormat;
	sourceContext.middleBufferContext.format = _phononStereo;
	sourceContext.outBufferContext.format = _phononStereo;

	sourceContext.inBuffer.resize(_frameSize * audioData->channelCount);
	sourceContext.middleBuffer.resize(_frameSize * 2);
	sourceContext.outBuffer.resize(_frameSize * 2);

	// Reset rest of source context
	sourceContext.inBufferContext.interleavedBuffer = &sourceContext.inBuffer[0];
	sourceContext.middleBufferContext.interleavedBuffer = &sourceContext.middleBuffer[0];
	sourceContext.outBufferContext.interleavedBuffer = &sourceContext.outBuffer[0];

	sourceContext.active = true;

	sourceContext.globalPosition = glm::vec3();
	sourceContext.globalRotation = glm::quat();

	sourceContext.soundSettings = sound->settings;
		
	//sourceContext.audioDecoder = audioDecoder;
	//sourceContext.dataView = data->createInstance();
	//sourceContext.dataView->mChannels = channels;
	sourceContext.audioData = audioData;
}

void Audio::receive(const entityx::ComponentRemovedEvent<Sound>& soundAddedEvent){
	auto sound = soundAddedEvent.component;

	// If sound never given source context, return
	if (sound->sourceContextIndex == -1)
		return;

	// Doing sound context stuff, so lock
	std::unique_lock<std::mutex> lock(_threadContext.mutex);

	// Clean up source context
	SourceContext& sourceContext = _threadContext.sourceContexts[sound->sourceContextIndex];

	sourceContext.active = false;

	// Delete to stop bleeding (probably should be tied to audio data instead!!!)
	iplDestroyBinauralEffect(&sourceContext.binauralObjectEffect);
	iplDestroyDirectSoundEffect(&sourceContext.directSoundEffect);
	sourceContext.binauralObjectEffect = nullptr;
	sourceContext.directSoundEffect = nullptr;

	sourceContext.audioData = nullptr;

	sourceContext.currentSample = 0;

	_threadContext.freeSourceContexts.push_back(sound->sourceContextIndex);
}

void Audio::receive(const CollidingEvent & collidingEvent){
	entityx::Entity soundEntity;
	
	if (collidingEvent.firstEntity.has_component<Sound>())
		soundEntity = collidingEvent.firstEntity;
	else if (collidingEvent.secondEntity.has_component<Sound>())
		soundEntity = collidingEvent.secondEntity;
	else
		return;
	
	if (collidingEvent.colliding)
		std::cout << "Colliding!" << std::endl;
	else
		std::cout << "Not colliding..." << std::endl;
}

void Audio::receive(const ContactEvent & contactEvent){
	entityx::Entity soundEntity;

	if (contactEvent.firstEntity.has_component<Sound>())
		soundEntity = contactEvent.firstEntity;
	else if (contactEvent.secondEntity.has_component<Sound>())
		soundEntity = contactEvent.secondEntity;
	else
		return;

	float impulse = 0.f;
	float distance = 0.f;
	
	for (uint8_t i = 0; i < contactEvent.contactCount; i++) {
		impulse += contactEvent.contacts[i].contactImpulse;
		distance += contactEvent.contacts[i].contactDistance;
	}
	
	std::cout << impulse << std::endl;
}