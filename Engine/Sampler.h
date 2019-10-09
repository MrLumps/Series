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
#include "Filter.h"
#include "Delay.h"
#include "Trigger.h"


using namespace std;




struct Sampler : EngineUnit {
private:
	const float minCutoff = 20.0f;
	const float maxCutoff = 10560.0f;

	array< Trigger, POLY> triggers;
	array<size_t, POLY> samplePos;

	int scopeIdx = 0;
	float scopeScale = 1.0f;
	float ratio = 0.0f;

public:

	array<float, POLY> playback_phase;

	Sampler(const float sampleRate_, shared_ptr<ModMatrix> matrix_, const int channel_) {
		sampleRate = sampleRate_;
		matrix = matrix_;
		channel = channel_;

		shittyscope.resize(1024, 0.0f);

		playback_phase.fill(1.0f);


		for (auto& p : samplePos) {
			p = numeric_limits<size_t>::max();
		}

		interface.consumers = {
			{ "Pan", 0.0f, 1.0f, nullptr, POLY },
			{ "Gain", 0.0f, 1.0f, nullptr, POLY },
		};

		registerInterface();


		for (int i = 0; i < POLY; i++) {
			interface.consumers[0].resP[i] = 0.5f; // set pan
		}

	}
	
	pair<float, float> process_audio(Kit &kit_, array<float, POLY> & gates_) {
		pair<float, float> sample = make_pair(0.0f, 0.0f);

		for (int i = 0; i < POLY; i++) {
			auto workingSample = make_pair(0.0f, 0.0f);

			if (kit_.current_samples[i]) {
				const float vca = rescale<float>(powf(50.0f, interface.consumers[1].resP[i]), 1.0f, 50.0f, 0.0f, 1.0f);

				if (triggers[i].process(gates_[i])) {
					samplePos[i] = 0;
				}
				
				if (samplePos[i] < kit_.current_samples[i]->size()) {
					workingSample = kit_.current_samples[i]->get(samplePos[i]);
					samplePos[i]++;
				}
				
				//playback_phase[i] = samplePos[i] / kit_.current_samples[i]->size(); //shame this has to go here now if I want blinkly things

				get<0>(workingSample) *= panL(interface.consumers[0].resP[i]) * vca;
				get<1>(workingSample) *= panR(interface.consumers[0].resP[i]) * vca;

			}

			get<0>(sample) += get<0>(workingSample);
			get<1>(sample) += get<1>(workingSample);

		}

		shittyscope[scopeIdx] = get<0>(sample);
		scopeIdx++;
		if (scopeIdx >= shittyscope.size()) {
			scopeIdx = 0;
		}

		return sample;

	}

	void loadConfig(const json cfg) {
		ModuleBase::loadConfig(cfg, false);
	}

};

