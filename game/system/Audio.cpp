#include "system\Audio.hpp"

#include "component\Name.hpp"
#include "component\Sound.hpp"
#include "component\Transform.hpp"

#include "other\Path.hpp"

#include <soundio\soundio.h>

#include <iostream>

#include <thread>
#include <atomic>
#include <mutex>

#include <audiodecoder.h>

//AudioDecoder* _audioDecoder;

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
	
	// testing sine wave
	//static float secondsOffset = 0.0f;
	//float secondsPerFrame = 1.f / outstream->sample_rate;
	//
	//float hz = 440.f;
	//float step = hz * glm::two_pi<float>();
	//float amps = 0.1f;
	
	int soundioError;
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

		if (!frameCount)
			break;

		threadContext.unmixedBuffers.clear();
	
		for (Audio::SourceContext& currentSourceContext : threadContext.sourceContexts) {
			if (!currentSourceContext.active)
				continue;
			
			// process sources
			//Audio::SourceContext& currentSourceContext = threadContext.sourceContexts[0];

			// resize buffers
			currentSourceContext.inBuffer.resize(frameCount);
			currentSourceContext.inBufferContext.interleavedBuffer = &currentSourceContext.inBuffer[0];
			currentSourceContext.inBufferContext.numSamples = frameCount;

			// sine samples
			//for (int frame = 0; frame < frameCount; frame += 1) 
			//	currentSourceContext.inBuffer[frame] = glm::sin((secondsOffset + frame * secondsPerFrame) * step) * amps;
			//
			//secondsOffset = glm::mod(secondsOffset + secondsPerFrame * frameCount, 1.0f);

			// pre-sampled data
			int framesGot = currentSourceContext.audioDecoder->read(frameCount, &currentSourceContext.inBuffer[0]);

			if (framesGot < frameCount) {
				currentSourceContext.audioDecoder->seek(0);
				currentSourceContext.audioDecoder->read(frameCount - framesGot, &currentSourceContext.inBuffer[framesGot]);
			}

			// prepare other buffers		
			threadContext.mixBufferContext.numSamples = frameCount;
			currentSourceContext.middleBufferContext.numSamples = frameCount;
			currentSourceContext.outBufferContext.numSamples = frameCount;

			// Update my source context position (not needed until environment data is given to phonon)
			//IPLDirectivity directivity;
			//directivity.dipoleWeight = 0;
			//directivity.dipolePower = 0;
			//
			//IPLSource source;
			//source.directivity = directivity;
			//source.position = toPhonon(currentSourceContext.globalPosition); // game units are cm, steam audio is in meters
			//source.ahead = toPhonon(currentSourceContext.globalRotation * Transform::forward);
			//source.up = toPhonon(currentSourceContext.globalRotation * Transform::up);
			//
			//IPLVector3 listenerGlobalPosition = toPhonon(threadContext.listenerGlobalPosition); // cm to meters
			//IPLVector3 listenerGlobalPositionForward = toPhonon(threadContext.listenerGlobalRotation * Transform::forward);
			//IPLVector3 listenerGlobalPositionUp = toPhonon(threadContext.listenerGlobalRotation * Transform::up);
			//
			//IPLDirectSoundPath soundPath = iplGetDirectSoundPath(threadContext.phononEnvironment, listenerGlobalPosition, listenerGlobalPositionForward, listenerGlobalPositionUp, source, currentSourceContext.radius, IPL_DIRECTOCCLUSION_NONE, IPL_DIRECTOCCLUSION_RAYCAST);

			// Apply distance attenutation
			IPLDirectSoundPath soundPath;

			float distance = glm::length(threadContext.listenerGlobalPosition - currentSourceContext.globalPosition);

			soundPath.distanceAttenuation = 1.f - (distance * 1.f / currentSourceContext.radius);

			if (soundPath.distanceAttenuation < 0.f)
				soundPath.distanceAttenuation = 0.f;
			else
				soundPath.distanceAttenuation = glm::pow(soundPath.distanceAttenuation, 3);

			iplApplyDirectSoundEffect(currentSourceContext.directSoundEffect, currentSourceContext.inBufferContext, soundPath, threadContext.effectOptions, currentSourceContext.middleBufferContext);

			// Unit vector from the listener to the point source, relative to the listener's coordinate system.
			IPLVector3 direction = toPhonon(glm::inverse(threadContext.listenerGlobalRotation) * glm::normalize(currentSourceContext.globalPosition - threadContext.listenerGlobalPosition));

			iplApplyBinauralEffect(currentSourceContext.binauralObjectEffect, threadContext.phononBinauralRenderer, currentSourceContext.middleBufferContext, direction, IPL_HRTFINTERPOLATION_BILINEAR, currentSourceContext.outBufferContext);
		
			threadContext.unmixedBuffers.push_back(currentSourceContext.outBufferContext);
		}

		iplMixAudioBuffers(threadContext.unmixedBuffers.size(), &threadContext.unmixedBuffers[0], threadContext.mixBufferContext);

		// copy over out buffer to outstream
		for (int frame = 0; frame < frameCount; frame += 1) {
			for (int channel = 0; channel < outstream->layout.channel_count; channel += 1) {
				float *ptr = (float*)(areas[channel].ptr + areas[channel].step * frame);

				//*ptr = currentSourceContext.inBuffer[frame]; // raw only
				//*ptr = threadContext.sourceContexts[0].middleBuffer[frame * 2 + channel]; // direct only
				*ptr = threadContext.mixBuffer[frame * 2 + channel]; // direct + binaural
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

Audio::Audio(const ConstructorInfo& constructorInfo) : _sampleRate(constructorInfo.sampleRate), _frameSize(constructorInfo.frameSize), _path(constructorInfo.path){
	// Create phonon context
	IPLerror phononError = iplCreateContext(&phononLog, nullptr, nullptr, &_phononContext); // context

	if (phononError != IPL_STATUS_SUCCESS) {
		std::cerr << "Audio iplCreateContext: " << phononErrorMsg(phononError) << std::endl;
		return;
	}

	// shared variables
	_threadContext.frameSize = _frameSize;

	_phononMono.channelLayoutType = IPL_CHANNELLAYOUTTYPE_SPEAKERS;
	_phononMono.channelLayout = IPL_CHANNELLAYOUT_MONO;
	_phononMono.channelOrder = IPL_CHANNELORDER_INTERLEAVED;

	_phononStereo.channelLayoutType = IPL_CHANNELLAYOUTTYPE_SPEAKERS;
	_phononStereo.channelLayout = IPL_CHANNELLAYOUT_STEREO;
	_phononStereo.channelOrder = IPL_CHANNELORDER_INTERLEAVED;

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

	// Create direct sound object
	//phononError = iplCreateDirectSoundEffect(_phononEnvironmentRenderer, _phononMono, _phononStereo, &_threadContext.phononDirectSoundEffect); // direct sound effect
	//
	//if (phononError != IPL_STATUS_SUCCESS) {
	//	std::cerr << "Audio iplCreateDirectSoundEffect: " << phononErrorMsg(phononError) << std::endl;
	//	return;
	//}
	
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

	std::cerr << "Audio soundio_get_output_device: " << _soundIoDevice->name << std::endl;

	// Initiate my source context (contains binaural object effect, audio buffers, and direct sound path)
	// testing, just the one for now
	//phononError = iplCreateBinauralEffect(_threadContext.phononBinauralRenderer, _phononStereo, _phononStereo, &_threadContext.sourceContext.binauralObjectEffect);
	//
	//if (phononError != IPL_STATUS_SUCCESS) {
	//	std::cerr << "Audio iplCreateBinauralEffect: " << phononErrorMsg(phononError) << std::endl;
	//	return;
	//}

	_threadContext.effectOptions.applyAirAbsorption = IPL_FALSE;
	_threadContext.effectOptions.applyDirectivity = IPL_FALSE;
	_threadContext.effectOptions.applyDistanceAttenuation = IPL_TRUE;
	_threadContext.effectOptions.directOcclusionMode = IPL_DIRECTOCCLUSION_NONE;

	// Testing load sound
	//_audioDecoder = new AudioDecoder(formatPath(_path, "retrotv.wav"));
	//_audioDecoder->open();

	// Create soundio out stream
	_soundIoOutStream = soundio_outstream_create(_soundIoDevice);
	_soundIoOutStream->format = SoundIoFormatFloat32NE;
	_soundIoOutStream->write_callback = writeCallback;
	_soundIoOutStream->userdata = &_threadContext; // set userdata to threadContext to retrieve inside thread
	_soundIoOutStream->sample_rate = _sampleRate;

	if (soundioError = soundio_outstream_open(_soundIoOutStream)) {
		std::cerr << "Audio soundio_outstream_open: " << soundio_strerror(soundioError) << std::endl;
		return;
	}

	if (_soundIoOutStream->layout_error)
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
	for (SourceContext& sourceContext : _threadContext.sourceContexts) {
		if (sourceContext.audioDecoder)
			delete sourceContext.audioDecoder;

		iplDestroyBinauralEffect(&sourceContext.binauralObjectEffect);
		iplDestroyDirectSoundEffect(&sourceContext.directSoundEffect);
	}

	iplDestroyEnvironmentalRenderer(&_phononEnvironmentRenderer);
	iplDestroyEnvironment(&_threadContext.phononEnvironment);
	iplDestroyBinauralRenderer(&_threadContext.phononBinauralRenderer);
	iplDestroyContext(&_phononContext);
}

void Audio::configure(entityx::EventManager & events){
	events.subscribe<ContactEvent>(*this);
	events.subscribe<CollidingEvent>(*this);
	events.subscribe< entityx::ComponentAddedEvent<Listener>>(*this);
	events.subscribe< entityx::ComponentAddedEvent<Sound>>(*this);
	events.subscribe< entityx::ComponentRemovedEvent<Sound>>(*this);
}

void Audio::update(entityx::EntityManager & entities, entityx::EventManager & events, double dt){
	if (!_listenerEntity.valid() || !_listenerEntity.has_component<Transform>() || !_listenerEntity.has_component<Listener>())
		return;

	std::unique_lock<std::mutex> lock(_threadContext.mutex);

	auto listenerTransform = _listenerEntity.component<Transform>();
	listenerTransform->globalDecomposed(&_threadContext.listenerGlobalPosition, &_threadContext.listenerGlobalRotation);

	//auto soundTransform = _soundEntity.component<Transform>();
	//soundTransform->globalDecomposed(&_threadContext.sourceContext.globalPosition, &_threadContext.sourceContext.globalRotation);

	for (auto entity : entities.entities_with_components<Transform, Sound>()) {
		auto transform = entity.component<Transform>();
		auto sound = entity.component<Sound>();

		SourceContext& sourceContext = _threadContext.sourceContexts[sound->sourceContextIndex];

		transform->globalDecomposed(&sourceContext.globalPosition, &sourceContext.globalRotation);
	}
}

void Audio::receive(const entityx::ComponentAddedEvent<Listener>& listenerAddedEvent){
	_listenerEntity = listenerAddedEvent.entity;
}

void Audio::receive(const entityx::ComponentAddedEvent<Sound>& soundAddedEvent) {
	auto sound = soundAddedEvent.component;

	std::unique_lock<std::mutex> lock(_threadContext.mutex);

	if (_threadContext.freeSourceContexts.size()) {
		sound->sourceContextIndex = *_threadContext.freeSourceContexts.rbegin();
		_threadContext.freeSourceContexts.pop_back();
	}
	else {
		sound->sourceContextIndex = _threadContext.sourceContexts.size();
		_threadContext.sourceContexts.resize(sound->sourceContextIndex + 1);
	}

	SourceContext& sourceContext = _threadContext.sourceContexts[sound->sourceContextIndex];

	if (!sourceContext.binauralObjectEffect) {
		IPLerror phononError = iplCreateBinauralEffect(_threadContext.phononBinauralRenderer, _phononStereo, _phononStereo, &sourceContext.binauralObjectEffect);

		if (phononError != IPL_STATUS_SUCCESS) {
			std::cerr << "Audio iplCreateBinauralEffect: " << phononErrorMsg(phononError) << std::endl;
			return;
		}

		phononError = iplCreateDirectSoundEffect(_phononEnvironmentRenderer, _phononMono, _phononStereo, &sourceContext.directSoundEffect); // direct sound effect
		
		if (phononError != IPL_STATUS_SUCCESS) {
			std::cerr << "Audio iplCreateDirectSoundEffect: " << phononErrorMsg(phononError) << std::endl;
			return;
		}

		sourceContext.inBufferContext.format = _phononMono;
		sourceContext.middleBufferContext.format = _phononStereo;
		sourceContext.outBufferContext.format = _phononStereo;

		sourceContext.middleBuffer.resize(_frameSize * 2);
		sourceContext.middleBufferContext.interleavedBuffer = &sourceContext.middleBuffer[0];

		sourceContext.outBuffer.resize(_frameSize * 2);
		sourceContext.outBufferContext.interleavedBuffer = &sourceContext.outBuffer[0];
	}
	
	sourceContext.globalPosition = glm::vec3();
	sourceContext.globalRotation = glm::quat();

	sourceContext.radius = 10000.f;

	sourceContext.inBuffer.clear();
	sourceContext.middleBuffer.clear();
	sourceContext.outBuffer.clear();

	sourceContext.active = true;
	
	sourceContext.audioDecoder = new AudioDecoder(formatPath(_path, sound->soundFile));
	sourceContext.audioDecoder->open();
}

void Audio::receive(const entityx::ComponentRemovedEvent<Sound>& soundAddedEvent){
	auto sound = soundAddedEvent.component;

	std::unique_lock<std::mutex> lock(_threadContext.mutex);

	SourceContext& sourceContext = _threadContext.sourceContexts[sound->sourceContextIndex];

	sourceContext.active = false;

	_threadContext.freeSourceContexts.push_back(sound->sourceContextIndex);
}

void Audio::receive(const CollidingEvent & collidingEvent){
	//if (collidingEvent.colliding)
	//	std::cout << "Colliding!\n";
	//else
	//	std::cout << "Not colliding...\n";
}

void Audio::receive(const ContactEvent & contactEvent){
	//std::cout << "Contact!\n";
}