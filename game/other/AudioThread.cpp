#include "other\AudioThread.hpp"

#include <phonon.h>
#include <soundio\soundio.h>

#include <iostream>

const IPLAudioFormat phononMono{ IPL_CHANNELLAYOUTTYPE_SPEAKERS, IPL_CHANNELLAYOUT_MONO, 0, 0, 0, (IPLAmbisonicsOrdering)0, (IPLAmbisonicsNormalization)0, IPL_CHANNELORDER_INTERLEAVED };

const IPLAudioFormat phononStereo{ IPL_CHANNELLAYOUTTYPE_SPEAKERS, IPL_CHANNELLAYOUT_STEREO, 0, 0, 0, (IPLAmbisonicsOrdering)0, (IPLAmbisonicsNormalization)0, IPL_CHANNELORDER_INTERLEAVED };

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

void fillBuffer(AudioInput* audioInput, uint32_t requestedSamples, uint32_t sampleRate, const AudioSource::SoundSettings settings, std::vector<float>* buffer) {

	// skip if non-looping sound reached end of sampleCount
	if (audioInput->currentSample == audioInput->sampleCount && !settings.loop) 
		return;

	uint32_t samplesLeft = requestedSamples;

	do {
		// reset currentSample for looping files
		if (audioInput->currentSample == audioInput->sampleCount) {
			assert(settings.loop); // non looping sounds should fall out of while loop
			audioInput->currentSample = 0;
		}

		// The last sample after requestedSamples
		uint32_t endSample = glm::min(audioInput->currentSample + requestedSamples, audioInput->sampleCount); // clamp to under sampleCount

		// The amount of samples after endSample
		uint32_t samplesGot = glm::min(endSample - audioInput->currentSample, samplesLeft); // clamp to under samplesLeft

		assert(samplesGot);
		
		// get samples
		const float* samplesPtr = nullptr;
		audioInput->inputCallback(audioInput->userData, sampleRate, audioInput->channels, samplesGot, audioInput->currentSample, &samplesPtr);

		assert(samplesGot && samplesPtr);
		
		uint32_t offsetCh = (requestedSamples * audioInput->channels) - (samplesLeft * audioInput->channels);

		// copy over samples
		for (uint32_t i = 0; i < samplesGot * audioInput->channels; i++) {
			float sample = samplesPtr[i] * settings.volume;

			(*buffer)[offsetCh + i] = sample;
		}

		audioInput->currentSample += samplesGot;

		// should never exceed sampleCount
		assert(audioInput->currentSample <= audioInput->sampleCount);

		// should never get back more samples than requested
		assert(samplesLeft >= samplesGot);

		samplesLeft -= samplesGot;

		// let non-looping file fall out with samplesLeft to fill,
		// or looping file loops back round and has currentSample set to 0
	} while (settings.loop && samplesLeft);
}

float calculateDistanceAttenutation(const glm::vec3& listener, const glm::vec3& source, float radius, float falloffPower) {
	float distance = glm::length(listener - source);

	if (distance >= radius)
		return 0;

	return glm::pow(glm::clamp(1.f - (distance * 1.f / radius), 0.f, 1.f), falloffPower);
}

void soundioWriteCallback(SoundIoOutStream* outstream, int frameCountMin, int frameCountMax) {
	AudioThreadContext* threadContext = (AudioThreadContext*)outstream->userdata;

	if (threadContext->error)
		return;
	
	// soundio vars
	SoundIoChannelArea* areas;
	int framesLeft = threadContext->frameSize;
	int soundioError;
	
	// phonon vars
	IPLAudioBuffer inputBufferContext;
	IPLAudioBuffer middleBufferContext;
	IPLAudioBuffer outputBufferContext;
	IPLAudioBuffer mixBufferContext;

	IPLDirectSoundEffectOptions directSoundOptions;

	std::vector<IPLAudioBuffer> unmixedBuffers;
	
	middleBufferContext.format = phononStereo;
	outputBufferContext.format = phononStereo;
	mixBufferContext.format = phononStereo;

	directSoundOptions.applyAirAbsorption = IPL_FALSE;
	directSoundOptions.applyDirectivity = IPL_FALSE;
	directSoundOptions.directOcclusionMode = IPL_DIRECTOCCLUSION_NONE;
	
	// lock thread
	std::unique_lock<std::mutex> threadLock(threadContext->threadLock);
	
	// if nothing to play, do nothing
	if (!threadContext->sourceContexts.size())
		return;
	
	while (framesLeft > 0) {
		// open outstream
		int frameCount = framesLeft;
	
		if (soundioError = soundio_outstream_begin_write(outstream, &areas, &frameCount)) {
			threadContext->error = true;
			std::cerr << "Audio soundio_outstream_begin_write: " << soundio_strerror(soundioError) << std::endl;
			return;
		}
	
		// resize mix buffer
		threadContext->mixBuffer.resize(frameCount * 2);
		mixBufferContext.interleavedBuffer = &threadContext->mixBuffer[0];
		mixBufferContext.numSamples = frameCount;
	
		// for each source context
		for (AudioThreadContext::SourceContext& sourceContext : threadContext->sourceContexts){
			//std::unique_lock<std::mutex> sourceLock(sourceContext.sourceLock); // lock source
		
			if (!sourceContext.valid)
				continue;
		
			const AudioSource& source = sourceContext.audioSource;
			const AudioSource::SoundSettings& sound = source.soundSettings;
			const AudioListener& listener = threadContext->listener;
		
			// if currentSample at end of sampleCount (or greater than, shouldn't be though)
			if (sourceContext.audioInput.currentSample >= sourceContext.audioInput.sampleCount)
				continue;
		
			// if source not playing
			if (!sourceContext.audioSource.soundSettings.playing)
				continue;
		
			// resize buffers and set phonon buffer contexts
			sourceContext.inBuffer.resize(frameCount * sourceContext.audioInput.channels); // input is either mono or stereo
			sourceContext.middleBuffer.resize(frameCount * 2); // always stereo after processing
			sourceContext.outBuffer.resize(frameCount * 2); 

			inputBufferContext.format = (sourceContext.audioInput.channels == 1 ? phononMono : phononStereo);

			inputBufferContext.interleavedBuffer = &sourceContext.inBuffer[0];
			middleBufferContext.interleavedBuffer = &sourceContext.middleBuffer[0];
			outputBufferContext.interleavedBuffer = &sourceContext.outBuffer[0];

			inputBufferContext.numSamples = frameCount;
			middleBufferContext.numSamples = frameCount;
			outputBufferContext.numSamples = frameCount;
					
			// get samples from callback
			fillBuffer(&sourceContext.audioInput, frameCount, threadContext->sampleRate, sound, &sourceContext.inBuffer);
		
			// Apply distance attenutation
			directSoundOptions.applyDistanceAttenuation = (sound.attenuated ? IPL_TRUE : IPL_FALSE);
		
			IPLDirectSoundPath soundPath;
			soundPath.distanceAttenuation = calculateDistanceAttenutation(listener.globalPosition, source.globalPosition, sound.radius, sound.falloffPower);
		
			// skip if outside radius
			if (soundPath.distanceAttenuation == 0)
				continue;
		
			iplApplyDirectSoundEffect(sourceContext.directSoundEffect, inputBufferContext, soundPath, directSoundOptions, middleBufferContext);
		
			// Apply binaural
			IPLVector3 direction = toPhonon(glm::inverse(listener.globalRotation) * glm::normalize(source.globalPosition - listener.globalPosition));
		
			iplApplyBinauralEffect(sourceContext.binauralObjectEffect, threadContext->phononBinauralRenderer, middleBufferContext, direction, IPL_HRTFINTERPOLATION_BILINEAR, outputBufferContext);
		
			unmixedBuffers.push_back(outputBufferContext);
		}
	
		// If buffers to mix, mix with phonon
		if (unmixedBuffers.size())
			iplMixAudioBuffers(unmixedBuffers.size(), &unmixedBuffers[0], mixBufferContext);
	
		// If mixed buffers, copy over buffer to outstream. Else copy over 0.f
		for (int frame = 0; frame < frameCount; frame += 1) {
			for (int channel = 0; channel < outstream->layout.channel_count; channel += 1) {
				float *ptr = (float*)(areas[channel].ptr + areas[channel].step * frame);
	
				if (unmixedBuffers.size())
					*ptr = threadContext->mixBuffer[frame * 2 + channel];
				else
					*ptr = 0.f;
			}
		}
	
		// close outstream
		if (soundioError = soundio_outstream_end_write(outstream)) {
			threadContext->error = true;
			std::cerr << "Audio soundio_outstream_end_write: " << soundio_strerror(soundioError) << std::endl;
			return;
		}
	
		framesLeft -= frameCount;
	}
}

void cleanupPhononSource(AudioThreadContext::SourceContext* sourceContext) {
	if (sourceContext->directSoundEffect)
		iplDestroyDirectSoundEffect(&sourceContext->directSoundEffect);

	if (sourceContext->binauralObjectEffect)
		iplDestroyBinauralEffect(&sourceContext->binauralObjectEffect);
}

void cleanupPhonon(AudioThreadContext* threadContext) {
	// cleanup any still valid sourcecontexts (should be cleaned up by user already)
	for (AudioThreadContext::SourceContext& source : threadContext->sourceContexts) {
		if (!source.valid)
			continue;
	
		cleanupPhononSource(&source);
	}

	// clean up phonon (if it was init)
	if (threadContext->phononEnvironmentRenderer)
		iplDestroyEnvironmentalRenderer(&threadContext->phononEnvironmentRenderer);

	if (threadContext->phononEnvironment)
		iplDestroyEnvironment(&threadContext->phononEnvironment);

	if (threadContext->phononBinauralRenderer)
		iplDestroyBinauralRenderer(&threadContext->phononBinauralRenderer);

	if (threadContext->phononContext)
		iplDestroyContext(&threadContext->phononContext);
}

bool initPhonon(AudioThreadContext* threadContext) {
	// Create phonon context
	IPLerror phononError;

	if (phononError = iplCreateContext(&phononLog, nullptr, nullptr, &threadContext->phononContext)) {
		cleanupPhonon(threadContext);
		std::cerr << "Audio iplCreateContext: " << phononErrorMsg(phononError) << std::endl;
		return false;
	}

	// shared variables
	IPLRenderingSettings settings{ threadContext->sampleRate, threadContext->frameSize, IPL_CONVOLUTIONTYPE_PHONON };

	// Create binaural renderer
	IPLHrtfParams hrtfParams{ IPL_HRTFDATABASETYPE_DEFAULT, nullptr, nullptr };

	if (phononError = iplCreateBinauralRenderer(threadContext->phononContext, settings, hrtfParams, &threadContext->phononBinauralRenderer)) {
		cleanupPhonon(threadContext);
		std::cerr << "Audio iplCreateBinauralRenderer: " << phononErrorMsg(phononError) << std::endl;
		return false;
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

	if (phononError = iplCreateEnvironment(threadContext->phononContext, nullptr, simulationSettings, nullptr, nullptr, &threadContext->phononEnvironment)) {
		cleanupPhonon(threadContext);
		std::cerr << "Audio iplCreateEnvironment: " << phononErrorMsg(phononError) << std::endl;
		return false;
	}

	// Create environment renderer
	if (phononError = iplCreateEnvironmentalRenderer(threadContext->phononContext, threadContext->phononEnvironment, settings, phononStereo, nullptr, nullptr, &threadContext->phononEnvironmentRenderer)) {
		cleanupPhonon(threadContext);
		std::cerr << "Audio iplCreateEnvironmentalRenderer: " << phononErrorMsg(phononError) << std::endl;
		return false;
	}

	return true;
}

bool initPhononSource(AudioThreadContext* threadContext, AudioThreadContext::SourceContext* sourceContext, uint8_t channels) {
	IPLerror phononError;
	IPLAudioFormat audioFormat = (channels == 1 ? phononMono : phononStereo);

	if (phononError = iplCreateDirectSoundEffect(threadContext->phononEnvironmentRenderer, audioFormat, phononStereo, &sourceContext->directSoundEffect)) {
		cleanupPhononSource(sourceContext);
		std::cerr << "Audio iplCreateDirectSoundEffect: " << phononErrorMsg(phononError) << std::endl;
		return false;
	}

	if (phononError = iplCreateBinauralEffect(threadContext->phononBinauralRenderer, phononStereo, phononStereo, &sourceContext->binauralObjectEffect)) {
		cleanupPhononSource(sourceContext);
		std::cerr << "Audio iplCreateBinauralEffect: " << phononErrorMsg(phononError) << std::endl;
		return false;
	}

	return true;
}

void cleanupSoundio(AudioThreadContext* threadContext) {
	// join callback thread (if soundio was init)
	if (threadContext->soundIo && threadContext->soundIoDevice && threadContext->soundIoOutStream) {
		soundio_wakeup(threadContext->soundIo);
		threadContext->audioThread.join();
	}

	// cleanup soundio (if it was init)
	if (threadContext->soundIoOutStream)
		soundio_outstream_destroy(threadContext->soundIoOutStream);

	if (threadContext->soundIoDevice)
		soundio_device_unref(threadContext->soundIoDevice);

	if (threadContext->soundIo)
		soundio_destroy(threadContext->soundIo);
}

bool initSoundio(AudioThreadContext* threadContext) {
	// Create soundio context
	threadContext->soundIo = soundio_create();

	if (!threadContext->soundIo)
		return false;

	int soundioError;

	if (soundioError = soundio_connect(threadContext->soundIo)) {
		cleanupSoundio(threadContext);
		std::cerr << "Audio soundio_connect: " << soundio_strerror(soundioError) << std::endl;
		return false;
	}

	soundio_flush_events(threadContext->soundIo);

	// Setup soundio device
	int defaultDeviceIndex = soundio_default_output_device_index(threadContext->soundIo);

	if (defaultDeviceIndex == -1) {
		cleanupSoundio(threadContext);
		std::cerr << "Audio soundio_default_output_device_index: no audio output devices" << std::endl;
		return false;
	}

	threadContext->soundIoDevice = soundio_get_output_device(threadContext->soundIo, defaultDeviceIndex);

	if (!threadContext->soundIoDevice) {
		cleanupSoundio(threadContext);
		std::cerr << "Audio soundio_get_output_device: couldn't get audio output device" << std::endl;
		return false;
	}

	std::cout << "Audio soundio_get_output_device: " << threadContext->soundIoDevice->name << std::endl;

	// Create soundio out stream
	threadContext->soundIoOutStream = soundio_outstream_create(threadContext->soundIoDevice);
	threadContext->soundIoOutStream->format = SoundIoFormatFloat32NE;
	threadContext->soundIoOutStream->write_callback = soundioWriteCallback;
	threadContext->soundIoOutStream->sample_rate = threadContext->sampleRate;
	threadContext->soundIoOutStream->userdata = threadContext;

	if (soundioError = soundio_outstream_open(threadContext->soundIoOutStream)) {
		cleanupSoundio(threadContext);
		std::cerr << "Audio soundio_outstream_open: " << soundio_strerror(soundioError) << std::endl;
		return false;
	}

	if (threadContext->soundIoOutStream->layout_error)
		std::cerr << "Audio soundio layout_error: " << soundio_strerror(threadContext->soundIoOutStream->layout_error) << std::endl;

	if (soundioError = soundio_outstream_start(threadContext->soundIoOutStream)) {
		cleanupSoundio(threadContext);
		std::cerr << "Audio soundio_outstream_start: " << soundio_strerror(soundioError) << std::endl;
		return false;
	}

	// startup thread
	threadContext->audioThread = std::thread(soundio_wait_events, threadContext->soundIo);

	return true;
}

bool createAudioThread(AudioThreadContext* threadContext, uint32_t sampleRate, uint32_t frameSize) {
	assert(threadContext);

	threadContext->sampleRate = sampleRate;
	threadContext->frameSize = frameSize;

	if (!initPhonon(threadContext) || !initSoundio(threadContext)) {
		destroyAudioThread(threadContext);
		return false;
	}

	return true;
}

void destroyAudioThread(AudioThreadContext* threadContext) {
	assert(threadContext);

	cleanupSoundio(threadContext);
	cleanupPhonon(threadContext);

	threadContext->error = false;
}

int createAudioSource(AudioThreadContext* threadContext, const AudioInput& audioInput, const AudioSource& audioSource) {
	assert(threadContext && audioInput.inputCallback && (audioInput.channels == 1 || audioInput.channels == 2));
	
	// reuse sourceindex if available, otherwise push new one
	uint32_t sourceIndex;
	
	std::unique_lock<std::mutex> threadLock(threadContext->threadLock); // lock thread

	if (!threadContext->freeSourceContexts.size()) {
		sourceIndex = threadContext->sourceContexts.size();
		threadContext->sourceContexts.resize(threadContext->sourceContexts.size() + 1);
	}
	else {
		// freeSourceContexts isn't used by thread, so no need to lock
		sourceIndex = *threadContext->freeSourceContexts.rbegin();
		threadContext->freeSourceContexts.pop_back();
	}
	
	// reset source context
	AudioThreadContext::SourceContext& sourceContext = threadContext->sourceContexts[sourceIndex];
	
	//std::unique_lock<std::mutex> sourceLock(sourceContext.sourceLock); // lock source
	
	assert(!sourceContext.valid); // sanity
	
	sourceContext.valid = true;
	sourceContext.audioSource = audioSource;
	sourceContext.audioInput = audioInput;
	
	// create phonon objects
	if (!initPhononSource(threadContext, &sourceContext, audioInput.channels)) {
		freeAudioSource(threadContext, sourceIndex);
		return -1;
	}
	
	return sourceIndex;
}

void freeAudioSource(AudioThreadContext* threadContext, int sourceIndex) {
	assert(threadContext && sourceIndex >= 0);

	std::unique_lock<std::mutex> threadLock(threadContext->threadLock); // lock thread
	
	AudioThreadContext::SourceContext& sourceContext = threadContext->sourceContexts[sourceIndex];
	
	//std::unique_lock<std::mutex> sourceLock(sourceContext.sourceLock); // lock source
	
	sourceContext.valid = false;
	cleanupPhononSource(&sourceContext);
	
	threadContext->freeSourceContexts.push_back(sourceIndex);
}

void setAudioListener(AudioThreadContext* threadContext, const AudioListener& listener) {
	assert(threadContext);

	// listener is used by by every source, so lock entire thread
	std::unique_lock<std::mutex> threadLock(threadContext->threadLock); // lock thread

	threadContext->listener = listener;
}

void setAudioSource(AudioThreadContext* threadContext, int sourceIndex, const AudioSource& audioSource) {
	assert(threadContext && sourceIndex >= 0);

	std::unique_lock<std::mutex> threadLock(threadContext->threadLock); // lock thread
	
	AudioThreadContext::SourceContext& sourceContext = threadContext->sourceContexts[sourceIndex];
	
	//std::unique_lock<std::mutex> sourceLock(sourceContext.sourceLock); // lock source
	
	sourceContext.audioSource = audioSource;
}