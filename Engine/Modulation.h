#pragma once
#include <array>
#include <cmath>
#include <memory>
#include <math.h>

#include "Common.h"
//#include "BaseObjects.h"




using namespace std;

//template <typename float, int 8> 
struct Lfo {
private:

	float dt;

public:
	float freqMin = static_cast<float>(0.001f);
	float freqMax = static_cast<float>(72.21f);

	array<float, POLY> phase;
	float* sine;
	//float* square;
	//float* saw;

	//0-1
	float* freq;


	Lfo(const float sampleRate_, float* sine_, float* freq_) {
		dt = 1.0f / sampleRate_;
		sine = sine_;
		freq = freq_;

		for (int i = 0; i < POLY; i++) {
			phase[i] = 0.0f;
			sine[i] = 0.0f;
			freq[i] = 0.0f;
		}
	}

	//0-1
	void process() {

		for (int i = 0; i < POLY; i++) {
			phase[i] += freqMin * powf(freqMax / freqMin, freq[i]) * dt;

			if (phase[i] >= 1.0) {
				phase[i] -= 1.0;
			}
			//square[i] = phase[i] < 0.5 ? 1.0 : 0.0;
		}

		//#pragma omp simd get 1200 even if I move approxSine in here manually
		for (int i = 0; i < POLY; i++) {
			sine[i] = (aproxSine(2.0f * PI * phase[i]) + 1.0f) * 0.5f;
			//saw[i] = 1.0f - phase[i];
		}




	}

};


//template <typename float, int POLY> 
struct Envelope {
private:
	const float maxLength = static_cast<float>(20.0);
	float sampleRate = static_cast<float>(44100.0);
	
	array<bool, POLY> decaying;
public:

	float* state   = nullptr;

	float* attack  = nullptr;
	float* decay   = nullptr;
	float* sustain = nullptr;
	float* release = nullptr;

	Envelope(const float sampleRate_, float* state_, float* attack_, float* decay_, float* sustain_, float* release_) {
		sampleRate = sampleRate_;
		state = state_;
		attack = attack_;
		decay = decay_;
		sustain = sustain_;
		release = release_;

		for (int i = 0; i < POLY; i++) {
			state[i] = 0.0;
			decaying[i] = false;
			attack[i] = 0.5;
			decay[i] = 0.5;
			sustain[i] = 0.5;
			release[i] = 0.5;
		}
	}

	void process(array<float, POLY> &in) {

		for (int i = 0; i < POLY; i++) {
			if (in[i] >= 1.0f) {
				if (decaying[i]) {
					state[i] += powf(20000.0f, 1.0f - decay[i]) / maxLength * (sustain[i] - state[i]) / sampleRate;
				}
				else {
					state[i] += powf(20000.0f, 1.0f - attack[i]) / maxLength * (1.01f - state[i]) / sampleRate;

					if (state[i] >= 1.0) {
						state[i] = 1.0;
						decaying[i] = true;
					}
				}
			}
			else {
				// Release
				state[i] += powf(20000.0f, 1 - release[i]) / maxLength * (0.0f - state[i]) / sampleRate;
				decaying[i] = false;
			}

		}
		
	}

};


//template <typename float, int 8> 
struct Noise {
private:
	uint32_t seed = 738;


	float getLcg() {
		seed = (seed >> 1) ^ (-(signed int)(seed & 1u) & 0xD0000001u);
		return static_cast<float>(2.32830643653869629E-10 * seed);
	}


public:
	float* noise = nullptr;

	Noise(const uint32_t seed, float* noise) : seed(seed), noise(noise) {
		for (int i = 0; i < POLY; i++) {
			noise[i] = getLcg();
		}
	}


	void process() {

		for (int i = 0; i < POLY; i++) {
			noise[i] = getLcg();
		}

	}

};



////template <typename float, int 8>
//struct pTrigger {
//private:
//	array<bool, POLY> state;
//	array<float, POLY> trigger;
//	const float lowfloathreshold = 0.0f;
//	const float highfloathreshold = 0.9f;
//public:
//
//	pTrigger() {
//		for (int i = 0; i < POLY; i++) {
//			state[i] = false;
//			trigger[i] = 0.0;
//		}
//	}
//
//	constexpr array<float, POLY>* get() {
//		return &trigger;
//	}
//
//	void process(array<float, POLY>& in) {
//		for (int i = 0; i < POLY; i++) {
//			trigger[i] = 0.0;
//			if (state[i]) { //High
//				if (in[i] <= lowfloathreshold) {
//					state[i] = false;
//				}
//			}
//			else {
//				if (in[i] >= highfloathreshold) {
//					state[i] = true;
//					trigger[i] = 1.0;
//				}
//			}
//		}
//	}
//
//};




//template <typename float, int 8> 
struct SandH {
private:
	
	array<Trigger, POLY> trigger;

public:
	float* state;
	float* sample;

	SandH(float* state, float* sample) : state(state), sample(sample) { 
		for (int i = 0; i < POLY; i++) {
			state[i] = 0.0f;
			sample[i] = 0.0f;
		}
	}

	void process(array<float, POLY>& in) {

		for (int i = 0; i < POLY; i++) {
			if(trigger[i].process(in[i])) {
				state[i] = sample[i];
			}
			
		}

	}

};




struct Modulation : ModuleBase {
private:
	unique_ptr<Lfo> lfo = nullptr;
	unique_ptr<Envelope> audioEnvelope = nullptr;
	//unique_ptr<Envelope> modEnvelope = nullptr;
	unique_ptr<Noise> noise = nullptr;
	unique_ptr<SandH> sandh = nullptr;

public:
	
	Modulation(const float sampleRate_, shared_ptr<ModMatrix> matrix_, const int channel_) {
		sampleRate = sampleRate_;
		matrix = matrix_;
		channel = channel_;

		interface.providers = {
			{ "Lfo",            0.0f, 1.0f, nullptr, POLY },
			{ "S & H",          0.0f, 1.0f, nullptr, POLY },
			{ "Noise",          0.0f, 1.0f, nullptr, POLY },
			{ "Audio Envelope", 0.0f, 1.0f, nullptr, POLY },
			{ "Gate",           0.0f, 1.0f, nullptr, POLY },
			{ "Const",          0.0f, 1.0f, nullptr, POLY }
		};
		
		interface.consumers = {
			{ "Lfo Freq",    0.0f, 1.0f, nullptr, POLY },
			{ "A Env Atk",   0.0f, 1.0f, nullptr, POLY },
			{ "A Env Dec",   0.0f, 1.0f, nullptr, POLY },
			{ "A Env Sus",   0.0f, 1.0f, nullptr, POLY },
			{ "A Env Rel",   0.0f, 1.0f, nullptr, POLY },
			{ "SandH Input", 0.0f, 1.0f, nullptr, POLY }
		};

		registerInterface();

		lfo = make_unique<Lfo>(sampleRate, interface.providers[0].resP, interface.consumers[0].resP);
		audioEnvelope = make_unique<Envelope>(sampleRate, 
			interface.providers[3].resP, 
			interface.consumers[1].resP, 
			interface.consumers[2].resP, 
			interface.consumers[3].resP, 
			interface.consumers[4].resP);
		noise = make_unique<Noise>(767, interface.providers[2].resP);
		sandh = make_unique<SandH>(interface.providers[1].resP, interface.consumers[5].resP);

		for (int i = 0; i < POLY; i++) {
			interface.providers[5].resP[i] = 1.0f;
		}

	}

	//Expects gates in
	void process(array<float, POLY>& in) {
		lfo->process();
		noise->process();
		audioEnvelope->process(in);
		sandh->process(in);

		for (int i = 0; i < POLY; i++) {
			interface.providers[4].resP[i] = in[i];
		}
	}

	//required because modulation isn't unloaded by channel when loading unlike the audio engine
	//void loadConfig(const json cfg) {
	//	ModuleBase::loadConfig(cfg, true);
	//}

};

