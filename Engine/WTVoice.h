#pragma once
#include <math.h>
#include <atomic>
#include <vector>
#include <condition_variable>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <memory>
#include <list>

#include "BaseObjects.h"
#include "Oscillators.h"


using namespace std;


struct WTVoice : EngineUnit {
private:
	array< unique_ptr<simplerWTableOsc>, POLY> oscillators; //Probably better to redo as a single unit

	//list< unique_ptr<AudioUnit> > fxChain;

	int scopeIdx = 0;
	float scopeScale = 1.0f;

public:

	WTVoice(const float sampleRate_, shared_ptr<ModMatrix> matrix_, const int channel_) {
		sampleRate = sampleRate_;
		matrix = matrix_;
		channel = channel_;

		shittyscope.resize(1024, 0.0f);

		for (auto& o : oscillators) {
			o = make_unique<simplerWTableOsc>(sampleRate);
		}

		//setVoiceFrequencies();

		interface.consumers = {
			{ "levelA", 0.0f, 1.0f,        nullptr, POLY },
			{ "levelB", 0.0f, 1.0f,        nullptr, POLY },
			{ "Detune", 0.0f, 1.0f,        nullptr, POLY },
			{ "Pan", 0.0f, 1.0f,           nullptr, POLY },
			{ "Filter",   0.0f, 1.0f,      nullptr, POLY },
			{ "Filter Type",   0.0f, 1.0f, nullptr, POLY },
			{ "Gain", 0.0f, 1.0f,          nullptr, POLY },
		};

		registerInterface();

		for (int i = 0; i < POLY; i++) {
			interface.consumers[0].resP[i] = 0.5f; // set levela
			interface.consumers[1].resP[i] = 0.5f; // set levelb
			interface.consumers[3].resP[i] = 0.5f; // set pan
			interface.consumers[4].resP[i] = 0.5f; // set filter
		}

	}


	//void setVoiceFrequencies() {
	//
	//	for (int i = 0; i < 8; i++) {
	//		oscillators[i]->setFreq(kit.tuning.freq(kit.notes[i]));
	//	}
	//
	//}


	pair<float, float> process_audio(Kit &kit_, array<float, POLY> & gates_) {
		const auto levelA		   = interface.consumers[0].resP;
		const auto levelB		   = interface.consumers[1].resP;
		const auto detune		   = interface.consumers[2].resP;
		const auto pan			   = interface.consumers[3].resP;
		const auto filterCutoff	   = interface.consumers[4].resP;
		const auto filterResponse  = interface.consumers[5].resP;
		const auto gain            = interface.consumers[6].resP;

		pair<float, float> sample = make_pair(0.0f, 0.0f);

		for (int i = 0; i < POLY; i++) {
			oscillators[i]->setFreq(kit_.tuning.freq(kit_.notes[i]));
			oscillators[i]->setDetune(detune[i]);
			oscillators[i]->setMix(levelA[i], levelB[i]);
			oscillators[i]->setFilterCutoff(filterCutoff[i]);
			oscillators[i]->setFilterType(filterResponse[i]);

			const float vca = rescale<float>(powf(50.0f, gain[i]), 1.0f, 50.0f, 0.0f, 1.0f);

			if (vca > 0.0f) {
				const float monoSample = oscillators[i]->wav() * vca;

				get<0>(sample) += monoSample * panL(pan[i]);
				get<1>(sample) += monoSample * panR(pan[i]);

			}

		}

		get<0>(sample) = tanh(get<0>(sample));
		get<1>(sample) = tanh(get<1>(sample));

		shittyscope[scopeIdx] = get<0>(sample);
		scopeIdx++;
		if (scopeIdx >= shittyscope.size()) {
			scopeIdx = 0;
		}

		return sample;
	}


};


