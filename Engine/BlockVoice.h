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
#include <utility>
#include <memory>
#include <thread>
#include <future>
#include <list>


#include "BaseObjects.h"
#include "Envelope.h"
#include "Delay.h"


using namespace std;


const std::vector<std::string> blockEffectList{
	"Delay",
	"BlackStar Reverb"
};




struct BlockOsc {

	float rPhase = 0.0f;
	float aPhase = 0.0f;
	float bPhase = 0.0f;
	float cPhase = 0.0f;
	float mPhase = 0.0f;

	float rDp = 0.0f;
	float aDp = 0.0f;
	float bDp = 0.0f;
	float cDp = 0.0f;
	float mDp = 0.0f;

	//LerpLine<float, 442> delay;
	DelayLine<float, 38> delay;
	StateVariableFilter2 res_filter;

	AR rootEnv;
	AR modeAEnv;
	AR modeBEnv;
	AR modeCEnv;

	float vibrato_depth = 0.0f;
	float vibrato_rate = 0.0f;

	////0-11
	//const std::vector<float> frequencies{
	//	198.499959f, 271.000005f, 292.000069f,	363.0f,	396.999919f, 443.000107f,	542.000011f,584.000139f, 726.0f, 793.999839f,	886.000214f,	1084.000022f
	//};
	//
	////Hopefully I've done this right and this will result in equal loudness, based on ITU 468
	//const std::vector<float> levelAdj{
	//	1.0f, 0.878183f, 0.703292f, 0.652725f, 0.538009f, 0.499793f, 0.454784f, 0.376163f, 0.349053f, 0.27843f, 0.25375f
	//};

	float dt = 0.0f;

	BlockOsc(const float sampleRate_) {
		dt = 1.0f / sampleRate_;

		rootEnv.setSampleRate(sampleRate_);
		modeAEnv.setSampleRate(sampleRate_);
		modeBEnv.setSampleRate(sampleRate_);
		modeCEnv.setSampleRate(sampleRate_);

		rPhase = 0.0f;
		aPhase = 0.0f;
		bPhase = 0.0f;
		cPhase = 0.0f;
		mPhase = 0.0f;

		delay.zero();
		res_filter.zero(sampleRate_);
		res_filter.setFreq(1.0f, 1.0f);
	}

	void triggerNote(const float hz_, const float attack_, const float decay_, const float vibrato_depth_, const float vibrato_rate_) {
		vibrato_depth = vibrato_depth_;

		rDp = hz_ * dt;
		aDp = hz_ * 2.76f * dt;
		bDp = hz_ * 5.40f * dt;
		cDp = hz_ * 8.93f * dt;
		mDp = vibrato_rate_ * dt;

		rootEnv.attack = attack_;
		rootEnv.release = decay_;

		modeAEnv.attack = attack_;
		modeAEnv.release = decay_ * 0.75f;

		modeBEnv.attack = attack_;
		modeBEnv.release = decay_ * 0.5f;

		modeCEnv.attack = attack_;
		modeCEnv.release = decay_ * 0.25f;

		rootEnv.trigger();
		modeAEnv.trigger();
		modeBEnv.trigger();
		modeCEnv.trigger();

		res_filter.setFreq(hz_, 1.0f + (vibrato_depth * 4.0f));

	}


	float process() {

		rootEnv.step();
		modeAEnv.step();
		modeBEnv.step();
		modeCEnv.step();

		const float rm = sinf(2 * PI * rPhase) * rootEnv.env();
		const float am = 0.5f * sinf(2 * PI * aPhase) * modeAEnv.env();
		const float bm = 0.25f * sinf(2 * PI * bPhase) * modeBEnv.env();
		const float cm = 0.25f * sinf(2 * PI * cPhase) * modeCEnv.env();
		const float mm = vibrato_depth * ((sinf(2 * PI * mPhase) + 1.0f ) * 0.5f);

		float mix = ((rm + am) + (bm + cm)) / ((1.0f + modeAEnv.env()) + (modeBEnv.env() + modeCEnv.env()));

		rPhase += rDp;
		aPhase += aDp;
		bPhase += bDp;
		cPhase += cDp;
		mPhase += mDp;

		if (rPhase > 1.0f) {
			rPhase -= 1.0f;
		}

		if (aPhase > 1.0f) {
			aPhase -= 1.0f;
		}

		if (bPhase > 1.0f) {
			bPhase -= 1.0f;
		}

		if (cPhase > 1.0f) {
			cPhase -= 1.0f;
		}

		if (mPhase > 1.0f) {
			mPhase -= 1.0f;
		}
		
		delay.set(mix);
		res_filter.process(delay.get());
		
		//res_filter.process(mix);
		//return res_filter.lowpass() * mm;
		
		//return delay.get();
		//return lerp<float>(mix, ((mix + res_filter.lowpass() ) * 0.5) * mm, vibrato_depth);
		return lerp<float>(mix, res_filter.lowpass() * mm, vibrato_depth);
		//return mix;

	}

};


struct BlockVoice : EngineUnit {
private:
	array< unique_ptr<BlockOsc>, POLY> oscillators; //Probably better to redo as a single unit

	int scopeIdx = 0;
	float scopeScale = 1.0f;

	array<Trigger, POLY> triggers;


public:

	BlockVoice(const float sampleRate_, shared_ptr<ModMatrix> matrix_, const int channel_) {
		sampleRate = sampleRate_;
		matrix = matrix_;
		channel = channel_;

		shittyscope.resize(1024, 0.0f);

		for (auto& o : oscillators) {
			o = make_unique<BlockOsc>(sampleRate);
		}

		//setVoiceFrequencies();

		interface.options = {
			{ "Attack", 0.0f, 0.6f,        nullptr, 1 },
			{ "Decay", 0.5f, 1.0f,         nullptr, 1 },
			{ "Vibrato Depth", 0.0f, 1.0f, nullptr, 1 },
			{ "Vibrato Rate", 0.0f, 15.0f, nullptr, 1 }
		};

		interface.consumers = {
			{ "Pan", 0.0f, 1.0f, nullptr, POLY }
		};

		registerInterface();

		for (int i = 0; i < POLY; i++) {
			interface.consumers[0].resP[i] = 0.5f; // set pan
		}

	}

	pair<float, float> process_audio(Kit& kit_, array<float, POLY>& gates_) {
		 const auto attack  = *interface.options[0].resP;
		 const auto decay   = *interface.options[1].resP;
		 const auto v_depth = *interface.options[2].resP;
		 const auto v_rate  = *interface.options[3].resP;

		const auto pan = interface.consumers[0].resP;



		pair<float, float> sample = make_pair(0.0f, 0.0f);


		for (int i = 0; i < POLY; i++) {
			if (triggers[i].process(gates_[i])) {
				oscillators[i]->triggerNote(kit_.tuning.freq(kit_.notes[i]), attack, decay, v_depth, v_rate);
			}

			const float monoSample = oscillators[i]->process();

			get<0>(sample) += monoSample * panL(pan[i]);
			get<1>(sample) += monoSample * panR(pan[i]);

		}

		shittyscope[scopeIdx] = get<0>(sample);
		scopeIdx++;
		if (scopeIdx >= shittyscope.size()) {
			scopeIdx = 0;
		}

		return sample;
	}

};