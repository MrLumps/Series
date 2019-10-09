#pragma once



// Todo:
// Sample manager base object
// mod matrix special slots for click-less muting
// Move scope(s) to matrix windows
// make some sort of return object by name thingie
// Sfx playback, ext by name, with pan, doppler? doppler would be awesome
// click-less audio shutdown

// Grand reformulation
// Front load stuff into Series.h
// Remove GUI from internals
// New organization:
//   8 Channel Mixer
//     Each channel will have
//       EQ
//       Level
//       Type 
//         Samples - Each will listen for envelopes per note (eg note 0 is sample 0)
//         Synth(s) 
//         Later - Special thing for long samples / voices
//         Sfx - priority based sample playback, possibly with per-sample position
//      Part
//        This will either be midi - cycle through envelopes
//        Or a step sequencer + clock dividers / euclidian divider with routing
//        With handy set playing part function
//          now
//          next beat
//          next part loop
//     Envelopes - Driven by the currently playing part
//       Pan - Make mono voices, keep stereo fx, make pan part of envelope, that way we can pan individual sounds
//       Possibly call this 'Character' ?s
//     Fx Chain, delay reverb etc - per sample? Hmmm that would be better obviously
//   ModMatrix - Simplify a bunch
//     Make static connections to each channel
//   Remove BaseObjects - create interface object based on old modMatrix interface

//Series
//+-----------------+
//|Clock            |
//|array Channel[8] |
//|                 |
//+-----------------+
//
//Channel
//+-----------------+
//|EQ               |
//|array Channel[8] |
//|                 |
//+-----------------+
//
// Mod Sources (per channel, per voice... so 8*8 = 40.... eeeeeeee)
//   1 Envelope
//   1 LFOs
//   1 S&H noise
//   1 S&H sine
//   1 noise
// Part[0..7].gate[0..4] -> modBlock[0..7].in[0..4] 
// modMatrix -> routes modulations at channel granularity for now
// modBlock.process()
// Type[0..7] if running Type->Voice[0..7]
// 
// Voice API
//   Listens to modBlock[0..7] trigger / 
//
// Run before channel so it can write to modBlock
// 
// Current Part -> gates
//              -> general controls
//              -> container for 'measures'
//
//
//




//#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
//#include <windows.h>
//#include <mmsystem.h>
#include <portaudio.h>
#include <math.h>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <regex>
#include <memory>


#include "Common.h"

#include "ModMatrix.h"
#include "BaseObjects.h"
#include "Channel.h"

#include "Clock.h"
#include "Modulation.h"
#include "Measures.h"
#include "SampleManager.h"
#include "Kits.h"
//#include "ClockUtilities.h"
//#include "NoteUtilities.h"
//#include "Pad.h"

//#include "WTVoice.h"
//#include "ResVoice.h"
//#include "NaturaReduction.h"
//#include "MonoVoice.h"
//#include "BlockVoice.h"






using namespace std;

struct Series {
private:
	PaStream *stream = nullptr;
	PaStreamParameters outputParameters;
	PaError err = 0;

	float dt = 0.0f;
	float sampleRate = 44100.0f;

	bool shutdownAudio = false;
	const float clicklessShutdownPhaseDelta = 1.0f / static_cast<float>(AUDIO_BLOCK_SIZE);
	float clicklessShutdownPhase = 0.0f;

	   	  	   	 
public:
	float masterLevel = 0.5f;

	unique_ptr<Clock> clock;
	shared_ptr<Measures> measures;
	//unique_ptr<Modulation> modulation;
	shared_ptr<ModMatrix> modMatrix;
	array<unique_ptr<Channel>, CHANNELS> channels;
	shared_ptr<SampleManager> sampleManager;
	shared_ptr<KitManager> kitManager;


	Series() {
		clock = nullptr;
		measures = nullptr;
		modMatrix = nullptr;
		for (auto& p : channels) {
			p = nullptr;
		}
		sampleManager = nullptr;

		err = Pa_Initialize();

		outputParameters.device = Pa_GetDefaultOutputDevice();
		outputParameters.channelCount = 2;
		outputParameters.sampleFormat = paFloat32;
		outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
		outputParameters.hostApiSpecificStreamInfo = NULL;

		if (err != paNoError) {
			cout << "Error starting portAudio" << endl;
		}

	}

	~Series() {
		Pa_Terminate();
		stream = nullptr;
		cout << "Audio Terminated" << endl;
	}

	void initAudio(const float sampleRate_ = 44100.0f) {
		sampleRate = sampleRate;

		clock = make_unique<Clock>("Master", sampleRate);
		modMatrix = make_shared<ModMatrix>(sampleRate);
		measures = make_shared<Measures>();
		sampleManager = make_shared<SampleManager>();
		kitManager = make_shared<KitManager>(sampleManager);
		

		int tmp = 0;
		for (auto& c : channels) {
			c = make_unique<Channel>(to_string(tmp), sampleRate, modMatrix, tmp, measures, sampleManager, kitManager);
			tmp++;
		}

	}
	   	  

	bool startSound() {
		PaError err = Pa_OpenStream(&stream, NULL, &outputParameters, sampleRate, AUDIO_BLOCK_SIZE, paNoFlag, &Series::paCallback, this);
		//PaError err = Pa_OpenDefaultStream(&stream, 0, 2, paFloat32, 44100.0, AUDIO_BLOCK_SIZE, &Series::paCallback, this);



		if (err != paNoError)
		{
			cout << "Error opening stream device" << endl;
			return false;
		}

		dt = 1.0f / static_cast<float>(sampleRate);

		err = Pa_SetStreamFinishedCallback(stream, &Series::paStreamFinished);

		if (err != paNoError)
		{
			Pa_CloseStream(stream);
			stream = nullptr;
			cout << "Error setting Finished Callback" << endl;
			return false;
		}

		err = Pa_StartStream(stream);

		if (err != paNoError)
		{
			Pa_CloseStream(stream);
			stream = nullptr;
			cout << "Error starting stream" << endl;
			return false;
		}

		shutdownAudio = false;
		clicklessShutdownPhase = 0.0f;

		return true;
	}


	//clickless global mute goes here
	void stopSound() {
		shutdownAudio = true;
		Pa_StopStream(stream);
		std::this_thread::sleep_for(std::chrono::microseconds(5000));
		Pa_CloseStream(stream);
	}


	void dumpConfig() {
		json conf;

		conf["clock"] = clock->toConfig();
		conf["measures"] = measures->toConfig();
		conf["kits"] = kitManager->toConfig();

		for (int i = 0; i < CHANNELS; i++) {
			conf["channels"].push_back(channels[i]->toConfig());
		}

		conf["modMatrix"] = modMatrix->toConfig();

		cout << conf.dump(4) << endl;
	}

	void saveConfig(const string filename_ = "series.json") {
		json conf;
		ofstream savefile;
		savefile.open(filename_);

		conf["clock"] = clock->toConfig();
		conf["measures"] = measures->toConfig();
		conf["kits"] = kitManager->toConfig();

		for (int i = 0; i < CHANNELS; i++) {
			conf["channels"].push_back(channels[i]->toConfig());
		}

		conf["modMatrix"] = modMatrix->toConfig();

		savefile << setw(4) << conf << endl;
		savefile.close();
	}

	void loadConfig(const string filename_ = "series.json") {
		//clear existing config
		//modMatrix->clear();  //Check this strategy is not causing leaking memory, think it's fine but...
		//clock->clear();

		cout << "Loading " << filename_ << endl;

		json config;
		ifstream savefile;
		savefile.open(filename_);
		if (savefile.good()) {
			savefile >> config;

			if (config.count("clock")) {
				clock->loadConfig(config["clock"]);
			}

			if (config.count("measures")) {
				measures->loadConfig(config["measures"]);
			}

			if (config.count("kits")) {
				kitManager->loadConfig(config["kits"]);
			}

			modMatrix->clear();

			for (int i = 0; i < CHANNELS; i++) { 
				channels[i]->loadConfig(config["channels"][i]);
			}


			if (config.count("modMatrix")) {
				modMatrix->loadConfig(config["modMatrix"]);
			}

		}
		savefile.close();

	}

  

	
	   	  
private:

	//Callback function wrapper for paCallbackMethod,
	//userData has been set to this
	static int paCallback(const void *inputBuffer, void *outputBuffer,
		unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags statusFlags,
		void *userData) {
		return ((Series*)userData)->paCallbackMethod(inputBuffer, outputBuffer, framesPerBuffer, timeInfo, statusFlags);
	}


	int paCallbackMethod(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags) {
		float *out = (float*)outputBuffer;
		size_t i;

		// Prevent unused variable warnings.
		(void)timeInfo;
		(void)statusFlags;
		(void)inputBuffer;

//		if (shutdownAudio) {
//
//			for (i = 0; i < framesPerBuffer; i++) {
//				pair<float, float> LR = mixer();
//				const auto cl = 1.0f - clicklessShutdownPhase;
//				*out++ = get<0>(LR) * cl;
//				*out++ = get<1>(LR) * cl;
//			}
//
//			clicklessShutdownPhase += clicklessShutdownPhaseDelta;
//			if (clicklessShutdownPhase > 1.0f)
//				clicklessShutdownPhase = 1.0f;
//
//		} else {
			for (i = 0; i < framesPerBuffer; i++) {
				pair<float, float> LR = mixer();
				*out++ = get<0>(LR);
				*out++ = get<1>(LR);
			}
//		}

		return paContinue;

	}

	void paStreamFinishedMethod() {
		cout << "Audio Stream Ended" << endl;
	}

	//Stream finished wrapper
	static void paStreamFinished(void* userData)
	{
		return ((Series*)userData)->paStreamFinishedMethod();
	}
	   	 
	 
	//Audio work goes here
	//
	//Probably need to get each channel to process a block at a time
	//That way perhaps threads will be worth while
	//
	//Also for threading, will need some form of syncronisation
	//to cope with the dependancy on the mod matrix
	//
	//
	//
	//Could we change the mod matrix consumer end point to become an assigned offset
	//to a buffer in mod matrix?
	//it would cut out a big bunch of copies
	//and possibly make block processing easier
	//
	//
	//
	//             clock
	//               |
	//            modMatrix                                      <-Copies Interface providers to consumers
	//               |
	//   +---------+--------+--------+--------+--------+         <- requires per sample tick and a frame of mod matrix data
	//   |         |	    |        |        |        |
	// channel0 channel1 channel2   ...      ...    channelN     <- runs interface providers (modulation sources)
	//                                                           <- also interface consumers via audio engine (1 sample delayed)
	//
	//
	// Perhaps I can run each channel in a thread, and have it wait on a mutex
	//  then call mix_audio which then triggers that mutex and gets the sample
	//
	// async is horrible without some breadth
	//
	// Perhaps we could run through a buffer and output something for options
	// and run consumers per channel
	// 
	// Or how can we get this to operate against a buffer?
	// Multi-pass
	// write out
	//
	// Currently *producer\
	//           *producer + buffer -> *consumer
	//           *producer/
	//
	// Currently *producer\
	//           *producer + buffer -> *consumer
	//           *producer/
	//
	// Buffered Frames
	//           *producer\
	//           *producer + buffer
	//           *producer/
	// Consumer->mod_matrix->get(id, frame) returns buffer value
	// Producer->mod_matrix->write(id, frame) sets buffer value
	// This way we could run through the options and such
	//
	//
	//
	pair<float, float> mixer() {

		const bool tick = clock->process();

		modMatrix->Process();
		
		pair<float, float> sample = make_pair(0.0f, 0.0f);

		for (auto& c : channels) {
			const auto s = c->mix_audio(tick);
			get<0>(sample) += get<0>(s);
			get<1>(sample) += get<1>(s);
		}

		return sample;
	}


};