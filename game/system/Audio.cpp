#include "system\Audio.hpp"

#include "component\Name.hpp"
#include "component\Sound.hpp"
#include "component\Transform.hpp"

#include <soundio\soundio.h>

#include <iostream>

#include <thread>
#include <atomic>
#include <mutex>

//#include <audiodecoder.h>

// Copied from phonon docs: positive x-axis pointing right, positive y-axis pointing up, and the negative z-axis pointing ahead.
IPLVector3 toPhonon(const glm::vec3& vector) {
	return { vector.x, vector.z, -vector.y };
}

void phononLog(char* message) {
	std::cerr << "Audio phonon: " << message << std::endl;
}

const char* phononErrorMsg(IPLerror error) {
	// Copied over from phonon comments
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
	static float secondsOffset = 0.0f;
	float secondsPerFrame = 1.f / outstream->sample_rate;

	float hz = 440.f;
	float step = hz * glm::two_pi<float>();
	float amps = 0.1f;
	
	int soundioError;

	while (framesLeft > 0) {
		// open outstream
		int frameCount = framesLeft;

		if (soundioError = soundio_outstream_begin_write(outstream, &areas, &frameCount)) {
			std::cerr << "Audio soundio_outstream_begin_write: " << soundio_strerror(soundioError) << std::endl;
			return;
		}

		if (!frameCount)
			break;
	
		// sine wave to in buffer (testing)
		threadContext.sourceContext.inBuffer.resize(frameCount);
		threadContext.sourceContext.inBufferContext.interleavedBuffer = &threadContext.sourceContext.inBuffer[0];
		threadContext.sourceContext.inBufferContext.numSamples = frameCount;

		for (int frame = 0; frame < frameCount; frame += 1) 
			threadContext.sourceContext.inBuffer[frame] = glm::sin((secondsOffset + frame * secondsPerFrame) * step) * amps;

		secondsOffset = glm::mod(secondsOffset + secondsPerFrame * frameCount, 1.0f);

		// prepare other buffers
		threadContext.sourceContext.middleBuffer.resize(frameCount * 2);
		threadContext.sourceContext.outBuffer.resize(frameCount * 2);

		threadContext.sourceContext.middleBufferContext.interleavedBuffer = &threadContext.sourceContext.middleBuffer[0];
		threadContext.sourceContext.middleBufferContext.numSamples = frameCount;

		threadContext.sourceContext.outBufferContext.interleavedBuffer = &threadContext.sourceContext.outBuffer[0];
		threadContext.sourceContext.outBufferContext.numSamples = frameCount;

		// Update my source context position
		std::unique_lock<std::mutex> lock(threadContext.positionMutex);

		IPLDirectivity directivity;
		directivity.dipoleWeight = 0;
		directivity.dipolePower = 0;

		threadContext.sourceContext.source.directivity = directivity;
		threadContext.sourceContext.source.position = toPhonon(threadContext.sourceGlobalPosition / 1000.f); // game units are cm, steam audio is in meters
		threadContext.sourceContext.source.ahead = toPhonon(threadContext.sourceGlobalRotation * Transform::down);
		threadContext.sourceContext.source.up = toPhonon(threadContext.sourceGlobalRotation * Transform::forward);
		
		// Calculate and apply direct sound effect
		IPLVector3 listenerGlobalPosition = toPhonon(threadContext.listenerGlobalPosition / 1000.f); // cm to meters
		IPLVector3 listenerGlobalPositionForward = toPhonon(threadContext.listenerGlobalRotation * Transform::down);
		IPLVector3 listenerGlobalPositionUp = toPhonon(threadContext.listenerGlobalRotation * Transform::forward);

		threadContext.sourceContext.soundPath = iplGetDirectSoundPath(threadContext.phononEnvironment, listenerGlobalPosition, listenerGlobalPositionForward, listenerGlobalPositionUp, threadContext.sourceContext.source, threadContext.sourceContext.radius, IPL_DIRECTOCCLUSION_NONE, IPL_DIRECTOCCLUSION_RAYCAST);

		iplApplyDirectSoundEffect(threadContext.phononDirectSoundEffect, threadContext.sourceContext.inBufferContext, threadContext.sourceContext.soundPath, threadContext.sourceContext.effectOptions, threadContext.sourceContext.middleBufferContext);
		
		// Unit vector from the listener to the point source, relative to the listener's coordinate system.
		IPLVector3 direction = toPhonon(glm::inverse(threadContext.listenerGlobalRotation) * glm::normalize(threadContext.sourceGlobalPosition - threadContext.listenerGlobalPosition));

		iplApplyBinauralEffect(threadContext.sourceContext.binauralObjectEffect, threadContext.phononBinauralRenderer, threadContext.sourceContext.middleBufferContext, direction, IPL_HRTFINTERPOLATION_BILINEAR, threadContext.sourceContext.outBufferContext);
		
		// copy over out buffer to outstream
		for (int frame = 0; frame < frameCount; frame += 1) {
			for (int channel = 0; channel < outstream->layout.channel_count; channel += 1) {
				float *ptr = (float*)(areas[channel].ptr + areas[channel].step * frame);
		
				//*ptr = threadContext.sourceContext.inBuffer[frame]; // raw only
				//*ptr = threadContext.sourceContext.middleBuffer[frame * 2 + channel]; // direct only
				*ptr = threadContext.sourceContext.outBuffer[frame * 2 + channel]; // direct + binaural
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

Audio::Audio() {
	// Create phonon context
	IPLerror phononError = iplCreateContext(&phononLog, nullptr, nullptr, &_phononContext); // context

	if (phononError != IPL_STATUS_SUCCESS) {
		std::cerr << "Audio iplCreateContext: " << phononErrorMsg(phononError) << std::endl;
		return;
	}

	// shared variables
	_phononMono.channelLayoutType = IPL_CHANNELLAYOUTTYPE_SPEAKERS;
	_phononMono.channelLayout = IPL_CHANNELLAYOUT_MONO;
	_phononMono.channelOrder = IPL_CHANNELORDER_INTERLEAVED;

	_phononStereo.channelLayoutType = IPL_CHANNELLAYOUTTYPE_SPEAKERS;
	_phononStereo.channelLayout = IPL_CHANNELLAYOUT_STEREO;
	_phononStereo.channelOrder = IPL_CHANNELORDER_INTERLEAVED;

	IPLRenderingSettings settings{ _sampleRate, _threadContext.frameSize, IPL_CONVOLUTIONTYPE_PHONON };

	// Create binaural renderer
	IPLHrtfParams hrtfParams{ IPL_HRTFDATABASETYPE_DEFAULT, nullptr, nullptr };
	phononError = iplCreateBinauralRenderer(_phononContext, settings, hrtfParams, &_threadContext.phononBinauralRenderer); // binaural renderer

	if (phononError != IPL_STATUS_SUCCESS) {
		std::cerr << "Audio iplCreateBinauralRenderer: " << phononErrorMsg(phononError) << std::endl;
		return;
	}

	// Create environment
	IPLSimulationSettings simulationSettings;
	simulationSettings.ambisonicsOrder = 0; // increases the amount of directional detail; must be between 0 and 3
	simulationSettings.bakingBatchSize = 0; // only used if sceneType is set to IPL_SCENETYPE_RADEONRAYS
	simulationSettings.irDuration = 0.5; // 0.5 to 4.0
	simulationSettings.irradianceMinDistance = 1; // Increasing this number reduces the loudness of reflections when standing close to a wall; decreasing this number results in a more physically realistic model
	simulationSettings.maxConvolutionSources = 16; // allows more sound sources to be rendered with sound propagation effects, at the cost of increased memory consumption
	simulationSettings.numBounces = 1; // 1 to 32
	simulationSettings.numDiffuseSamples = 32; // 32 to 4096
	simulationSettings.numOcclusionSamples = 32; // 32 to 512
	simulationSettings.numRays = 1024; // 1024 to 131072

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
	phononError = iplCreateDirectSoundEffect(_phononEnvironmentRenderer, _phononMono, _phononStereo, &_threadContext.phononDirectSoundEffect); // direct sound effect

	if (phononError != IPL_STATUS_SUCCESS) {
		std::cerr << "Audio iplCreateDirectSoundEffect: " << phononErrorMsg(phononError) << std::endl;
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
	int default_out_device_index = soundio_default_output_device_index(_soundIo);

	if (default_out_device_index < 0)
		return;

	_soundIoDevice = soundio_get_output_device(_soundIo, default_out_device_index);

	if (!_soundIoDevice)
		return;

	std::cerr << "Audio soundio_get_output_device: " << _soundIoDevice->name << std::endl;

	// Create soundio out stream
	_soundIoOutStream = soundio_outstream_create(_soundIoDevice);
	_soundIoOutStream->format = SoundIoFormatFloat32NE;
	_soundIoOutStream->write_callback = writeCallback;
	_soundIoOutStream->userdata = &_threadContext; // set userdata to threadContext object

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

	// Initiate my source context (contains binaural object effect, audio buffers, and direct sound path)
	phononError = iplCreateBinauralEffect(_threadContext.phononBinauralRenderer, _phononStereo, _phononStereo, &_threadContext.sourceContext.binauralObjectEffect);

	if (phononError != IPL_STATUS_SUCCESS) {
		std::cerr << "Audio iplCreateBinauralEffect: " << phononErrorMsg(phononError) << std::endl;
		return;
	}

	_threadContext.sourceContext.inBufferContext.format = _phononMono;
	_threadContext.sourceContext.middleBufferContext.format = _phononStereo;
	_threadContext.sourceContext.outBufferContext.format = _phononStereo;

	_threadContext.sourceContext.effectOptions.applyAirAbsorption = IPL_FALSE;
	_threadContext.sourceContext.effectOptions.applyDirectivity = IPL_FALSE;
	_threadContext.sourceContext.effectOptions.applyDistanceAttenuation = IPL_FALSE;
	_threadContext.sourceContext.effectOptions.directOcclusionMode = IPL_DIRECTOCCLUSION_NONE;
	
	_threadContext.sourceContext.radius = 10;

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
	iplDestroyBinauralEffect(&_threadContext.sourceContext.binauralObjectEffect);

	iplDestroyDirectSoundEffect(&_threadContext.phononDirectSoundEffect);
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
}

void Audio::update(entityx::EntityManager & entities, entityx::EventManager & events, double dt){
	if (!_listenerEntity.valid() || !_listenerEntity.has_component<Transform>() || !_listenerEntity.has_component<Listener>() ||
		!_soundEntity.valid() || !_soundEntity.has_component<Transform>() || !_soundEntity.has_component<Sound>())
		return;

	std::unique_lock<std::mutex> lock(_threadContext.positionMutex);

	auto listenerTransform = _listenerEntity.component<Transform>();
	listenerTransform->globalDecomposed(&_threadContext.listenerGlobalPosition, &_threadContext.listenerGlobalRotation);

	auto soundTransform = _soundEntity.component<Transform>();
	soundTransform->globalDecomposed(&_threadContext.sourceGlobalPosition, &_threadContext.sourceGlobalRotation);
}

void Audio::receive(const entityx::ComponentAddedEvent<Listener>& listenerAddedEvent){
	_listenerEntity = listenerAddedEvent.entity;
}

void Audio::receive(const entityx::ComponentAddedEvent<Sound>& soundAddedEvent) {
	_soundEntity = soundAddedEvent.entity;
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