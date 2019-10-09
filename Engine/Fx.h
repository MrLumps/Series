#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "BaseObjects.h"
#include "Filter.h"
#include "Delay.h"
#include "ModMatrix.h"
#include "Modulation.h"
#include "Oscillators.h"
#include "Trigger.h"
#include "Reverb.h"
#include "SampleManager.h"
#include "ImprovedMoog.h"

#include "Kits.h"
#include "clouds/dsp/granular_processor.h"
#include "elements/dsp/fx/reverb.h"


//A signal componant contains 4 things
//A buffer of type T and length S
//A function generate() which will fill that buffer
//A function display() which will call imgui stuff
//Some sort of arbitrary length render function or something

using namespace std;

//add stereo controlls
struct DelayEffect : AudioUnit {
private:
	LerpLine<float, 24000> delayL;
	LerpLine<float, 24000> delayR;

public:

	DelayEffect(const float sampleRate_, shared_ptr<ModMatrix> matrix_, const int channel_) {
		sampleRate = sampleRate_;
		matrix = matrix_;
		channel = channel_;

		interface.options = {
			{ "Dry/Wet Mix", 0.0f, 1.0f, nullptr, 1 },
			{ "Feedback", 0.0f, 1.0f,    nullptr, 1 },
			{ "Length", 0.0f, 1.0f,      nullptr, 1 }
		};

		registerInterface();
		
		delayL.zero();
		delayR.zero();

		
	}

	void process_audio(pair<float, float>& in_) {

		const auto echoL = get<0>(in_) + delayL.tap(*interface.options[2].resP) * *interface.options[1].resP;
		const auto echoR = get<1>(in_) + delayR.tap(*interface.options[2].resP) * *interface.options[1].resP;
		
		delayL.set(echoL);
		delayR.set(echoL);
		
		in_ = make_pair(lerp<float>(get<0>(in_), echoL, *interface.options[0].resP), lerp<float>(get<1>(in_), echoR, *interface.options[0].resP));

	}

};






#define IRBSBLOCKSIZE 1024

//struct BlackStarEffect : AudioUnit {
//private:
//	//std::vector< std::unique_ptr<IRKernel> >spaces;
//	unique_ptr<IRKernel<IRBSBLOCKSIZE>> kernel;
//
//	std::vector<pair<float,float>> inputBuffer;
//	std::vector<pair<float,float>> outputBuffer;
//	size_t bufferIdx = 0;
//	shared_ptr<SampleManager> sampleManager;
//	shared_ptr<Sample> sampleToload;
//	string current_sample_name;
//	
//
//public:
//
//	BlackStarEffect(const float sampleRate_, shared_ptr<ModMatrix> matrix_, shared_ptr<SampleManager> sampleManager_, const int channel_) {
//		sampleRate = sampleRate_;
//		matrix = matrix_;
//		sampleManager = sampleManager_;
//		channel = channel_;
//		
//		kernel = make_unique< IRKernel<IRBSBLOCKSIZE> >();
//
//		interface.options = {
//			{ "Dry/Wet Mix", 0.0f, 1.0f, nullptr, 1 }
//		};
//
//		registerInterface();
//
// 		inputBuffer.resize(IRBSBLOCKSIZE, make_pair(0.0f, 0.0f));
//		outputBuffer.resize(IRBSBLOCKSIZE, make_pair(0.0f, 0.0f));
//		bufferIdx = 0;
//		sampleToload = nullptr;
//		current_sample_name = "";
//	}
//
//	void loadSample(shared_ptr<Sample> sample_) {
//		sampleToload = sample_;
//	}
//
//	void process_audio(pair<float, float>& in_) {
//			   
//		if (bufferIdx < inputBuffer.size()-1) {
//			inputBuffer[bufferIdx] = in_;
//			bufferIdx++;
//		} else {
//			inputBuffer[bufferIdx] = in_;
//
//			if (sampleToload) {
//				kernel->loadSample(sampleToload);
//				current_sample_name = sampleToload->getName();
//				sampleToload = nullptr;
//			}
//			// Convolve block
//			kernel->processBlock(inputBuffer, outputBuffer);
//			bufferIdx = 0;
//		}
//
//
//
//		in_ = make_pair(
//			lerp<float>(get<0>(in_), get<0>(outputBuffer[bufferIdx]), *interface.options[0].resP),
//			lerp<float>(get<1>(in_), get<1>(outputBuffer[bufferIdx]), *interface.options[0].resP)
//		);
//	}
//
//	virtual json toConfig() {
//		json config = ModuleBase::toConfig();
//
//		config["sample_name"] = current_sample_name;
//
//		return config;
//	}
//
//	virtual void loadConfig(const json cfg) {
//		ModuleBase::loadConfig(cfg);
//
//		if (cfg.count("sample_name")) {
//			current_sample_name = cfg["sample_name"].get<string>();
//			if (sampleManager->getSample(current_sample_name)->size() > IRBSBLOCKSIZE) {
//				kernel->loadSample(sampleManager->getSample(current_sample_name));
//			}
//			else {
//				current_sample_name = "";
//			}
//		}
//
//	}
//
//
//	string currentSample() {
//		return current_sample_name;
//	}
//
//
//};










struct ChorusEffect : AudioUnit {
private:

	LowPass filterL;
	LowPass filterR;

	basicOsc lfoL;
	basicOsc lfoR;

	LerpLine<float, 1102> delayL;
	LerpLine<float, 1102> delayR;

public:

	ChorusEffect(const float sampleRate_, shared_ptr<ModMatrix> matrix_, const int channel_) {
		sampleRate = sampleRate_;
		matrix = matrix_;
		channel = channel_;

		interface.options = {
			{ "Dry/Wet Mix", 0.0f, 1.0f, nullptr, 1 },
			{ "Rate", 0.001f, 10.0f, nullptr, 1 },
			{ "Depth", 0.0f, 1.0f,   nullptr, 1 },
		};

		registerInterface();

		filterL.setFreq(10000.0f, sampleRate);
		filterR.setFreq(10000.0f, sampleRate);

		lfoL.zero(sampleRate);
		lfoR.zero(sampleRate);

		delayL.zero();
		delayR.zero();

	}

	void process_audio(pair<float, float>& in_) {

		lfoL.freq = *interface.options[1].resP;
		lfoR.freq = *interface.options[1].resP * 1.11111f;

		//lfoL.level = depth;
		//lfoR.level = depth;

		delayL.set(get<0>(in_));
		delayR.set(get<1>(in_));

		const auto delayLenL = 0.5f + (*interface.options[2].resP * lfoL.sineDC()) * 0.5f;
		const auto delayLenR = 0.5f + (*interface.options[2].resP * lfoR.sineDC()) * 0.5f;

		const auto mixL = lerp(get<0>(in_), delayL.tap(delayLenL), *interface.options[0].resP);
		const auto mixR = lerp(get<1>(in_), delayR.tap(delayLenR), *interface.options[0].resP);

		in_ = make_pair(mixL, mixR);

	}

	//Do nothing, dodge some api changes
	//void loadConfig(const json cfg, const bool do_register = false) {
	//	;
	//}
	//should be caught up

};















struct Phaser {
private:

	std::vector<AllPassFilter> delays;
	basicOsc lfo;
	float sampleRate = 44100.0f;
	float minMod = 0.0f;
	float maxMod = 0.0f;
	float modAmount = 0.0f;
	float last = 0.0f;
	float feedback = 0.0f;

public:

	Phaser(const float sampleRate_) {
		sampleRate = sampleRate_;
		delays.resize(6);
		lfo.zero(sampleRate_);
		lfo.freq = 0.25f;
		setRange(440.0f, 1600.0f);
	}

	void setRange(const float min_, const float max_) {
		minMod = min_ / (sampleRate * 0.5f);
		maxMod = max_ / (sampleRate * 0.5f);
	}

	//0-1
	void setModulationDepth(const float d_) {
		modAmount = d_;
	}

	//0-1
	void setFeedback(const float f_) {
		feedback = f_;
	}

	//Hz
	void setModulationRate(const float hz_) {
		lfo.freq = hz_;
	}

	float process(const float dt_, const float in_) {
		const float modulation = minMod + (maxMod - minMod) * lfo.sineDC();

		for (auto& d : delays) {
			d.setResponse(modulation);
		}

		float tmp = delays[0].process(
			delays[1].process(
				delays[2].process(
					delays[3].process(
						delays[4].process(
							delays[5].process(in_ + last * feedback))))));
		last = tmp;
		return (in_ + tmp * modAmount) / (1.0f + feedback);

	}



};






struct PhaserEffect : AudioUnit {
private:
	unique_ptr<Phaser> phaserL;
	unique_ptr<Phaser> phaserR;

	float dt = 1.0f;

	inline float detuneMul(const float cents_) {
		return round(100000.0f * pow(2.0f, (floor(cents_) / 100.0f / 12.0f))) / 100000.0f;
	}


public:

	PhaserEffect(const float sampleRate_, shared_ptr<ModMatrix> matrix_, const int channel_) {
		sampleRate = sampleRate_;
		matrix = matrix_;
		channel = channel_;

		interface.options = {
			{ "Dry/Wet Mix", 0.0f, 1.0f, nullptr, 1 },
			{ "Depth", 0.0f, 1.0f,       nullptr, 1 },
			{ "Decay", 0.0f, 1.0f,       nullptr, 1 }
		};

		registerInterface();

		phaserL = make_unique<Phaser>(sampleRate);
		phaserR = make_unique<Phaser>(sampleRate);

		phaserL->setModulationRate(0.2499f);
		phaserR->setModulationRate(0.2501f);

		dt = 1.0f / sampleRate;
	}



	void process_audio(pair<float, float>& in_) {
		phaserL->setModulationDepth(*interface.options[1].resP);
		phaserR->setModulationDepth(*interface.options[1].resP);

		phaserL->setFeedback(*interface.options[2].resP);
		phaserR->setFeedback(*interface.options[2].resP);

		const auto L = lerp<float>(get<0>(in_), phaserL->process(dt, get<0>(in_)), *interface.options[0].resP);
		const auto R = lerp<float>(get<1>(in_), phaserR->process(dt, get<1>(in_)), *interface.options[0].resP);
		
		in_ = make_pair(L, R);
	}

};



struct FilterEffect : AudioUnit {
private:
	StereoStateVariableFilter2 filter;

	const float minCutoff = 20.0f;
	const float maxCutoff = 10560.0f;
	
public:

	FilterEffect(const float sampleRate_, shared_ptr<ModMatrix> matrix_, const int channel_) {
		sampleRate = sampleRate_;
		matrix = matrix_;
		channel = channel_;

		filter.zero(sampleRate_);

		interface.options = {
			{ "Cutoff", 0.0f, 1.0f,   nullptr, 1 },
			{ "Response", 0.0f, 3.0f, nullptr, 1 },
			{ "balance", 0.0f, 1.0f,  nullptr, 1 }
		};

		registerInterface();

	}

	void process_audio(pair<float, float>& in_) {

		filter.setFreq(minCutoff * powf(maxCutoff / minCutoff, *interface.options[0].resP), *interface.options[2].resP);
		
		filter.process(in_);

		//Mix between lowpass/band pass/notch/high pass
		if (*interface.options[1].resP <= 1.0f) {
			const auto lp = filter.lowpass();
			const auto bp = filter.bandpass();

			get<0>(in_) = lerp<float>(get<0>(lp), get<0>(bp), *interface.options[1].resP);
			get<1>(in_) = lerp<float>(get<1>(lp), get<1>(bp), *interface.options[1].resP);
		} else if (*interface.options[1].resP <= 2.0f) {
			const auto bp = filter.bandpass();
			const auto np = filter.notch();

			get<0>(in_) = lerp<float>(get<0>(bp), get<0>(np), *interface.options[1].resP - 1.0f);
			get<1>(in_) = lerp<float>(get<1>(bp), get<1>(bp), *interface.options[1].resP - 1.0f);
		} else { //if (response <= 3.0f) {
			const auto np = filter.notch();
			const auto hp = filter.highpass();

			get<0>(in_) = lerp<float>(get<0>(np), get<0>(np), *interface.options[1].resP - 2.0f);
			get<1>(in_) = lerp<float>(get<1>(hp), get<1>(hp), *interface.options[1].resP - 2.0f);

		}

	}

};























template <typename T, int S> struct universalCombFilter {
private:
	LerpLine<T, S> delay;
	float bl = 0.0f;  //blend
	float fb = 0.0f;  //feedback
	float ff = 0.0f;  //feed forward
	float delay_len = 1.0f;

public:
	void zero() {
		bl = 0.0f;
		fb = 0.0f;
		ff = 0.0f;
		delay_len = 1.0f;
		delay.zero();
	}


	//gain 0-1
	void setFIR(const float g_) {
		bl = 1.0f;
		fb = 0.0f;
		ff = g_;
	}

	//scaling 0-1 and fb gain 0 - 1
	//.5 and .5 seem nice
	void setIIR(const float c_, const float g_) {
		bl = c_;
		fb = g_;
		ff = g_;
	}

	void setAllPass(const float coeff_) {
		bl = coeff_;
		fb = -coeff_;
		ff = 1.0f;
	}

	//0-1
	void setDelay(const float d_) {
		delay_len = d_;
	}

	float process(const float in_) {
		const float delayed = delay.tap(delay_len);
		const float xh = in_ + fb * delayed;
		delay.set(xh);
		return ff * delayed + bl * xh;
	}


};


//Comb + one pole lp
template <typename T, int S> struct lpCombFilter {
private:
	LerpLine<T, S> delay;

	//Set for IIR
	float bl = 0.5f;
	float fb = 0.5f;
	float ff = 0.0f;

	float o = 0.0f;
	float h = 0.0f;
	float state = 0.0f;
	float hp = 0.0f;
	float lp = 0.0f;
	float delay_len = 1.0f;
public:

	void zero() {
		bl = 0.5f;
		fb = 0.5f;
		ff = 0.0f;
		o = 0.0f;
		h = 0.0f;
		state = 0.0f;
		hp = 0.0f;
		lp = 0.0f;
		delay_len = 1.0f;
		delay.zero();
	}

	//r = Fr / Fs... 0-0.5
	void setResp(const float r_) {
		o = tan(r_ * M_PI);
		h = 1.0f / 1.0f + o;
	}

	//0-1
	void setDelay(const float d_) {
		delay_len = d_;
	}

	float process(const float in_) {
		const float delayed = delay.tap(delay_len);
		hp = h * ((fb * delayed) - o * state);
		lp = o * hp + state;
		state = o * lp;
		const float xh = in_ + lp;
		delay.set(xh);
		return ff * delayed + bl * xh;
	}


};









struct EnvFollower {
private:
	StateVariableFilter smoother;
	float state;

public:

	void zero(const float sampleRate_) {
		state = 0.0f;
		smoother.zero(sampleRate_);
		smoother.setFreq(10.0f);
	}

	//0-1
	void smoothness(const float s_) {
		smoother.setFreq(10.0f * powf(4000.0f / 10.0f, s_));
	}

	void setFreq(const float hz_) {
		smoother.setFreq(hz_);
	}

	//-1 - 1
	float process(const float in_) {
		float signal = abs(in_);
		signal *= signal;
		smoother.process(signal);
		state = tanh(smoother.lowpass());
		return state;
	};

	float envelope() {
		return state;
	}

};




struct PitchShifter {
	LerpLine<float, 3072> l;
	LerpLine<float, 3072> r;
	float phase = 0.0f;
public:

	void zero() {
		l.zero();
		r.zero();
		phase = 0.0f;
	}

	void set(const pair<float, float> in_) {
		l.set(get<0>(in_), 0.5f);
		r.set(get<1>(in_), 0.5f);
	}

	pair<float, float> shifted(const float ratio_) {
		pair<float, float> out = make_pair(0.0f, 0.0f);
		const float tri = 2.0f * (phase >= 0.5f ? 1.0f - phase : phase);

		float half = phase * 0.5f;

		get<0>(out) = l.tap(phase) * tri;
		get<0>(out) += l.tap(half) * (1.0f - tri);

		get<1>(out) = r.tap(phase) * tri;
		get<1>(out) += r.tap(half) * (1.0f - tri);

		phase += (1.0f - ratio_) / 3072.0f;

		if (phase >= 1.0f) {
			phase -= 1.0f;
		}

		if (phase <= 0.0f) {
			phase += 1.0f;
		}

		return out;

	}



};






struct ShifterEffect : AudioUnit {
private:
	PitchShifter shifter;

public:

	ShifterEffect(const float sampleRate_, shared_ptr<ModMatrix> matrix_, const int channel_) {
		sampleRate = sampleRate_;
		matrix = matrix_;
		channel = channel_;

		shifter.zero();

		interface.options = {
			{ "Ratio", -1.0f, 3.0f, nullptr, 1 }
		};

		registerInterface();

	}

	void process_audio(pair<float, float>& in_) {

		shifter.set(in_);
		in_ = shifter.shifted(*interface.options[0].resP);

	}

};


















/*
//Chunky but working
struct ReverbEffect : AudioUnit {
private:

	const float minCutoff = 40.0;
	const float maxCutoff = 7350.0;
	const float lfoAfreq = 0.2f; //hz
	const float lfoBfreq = 0.2333f; //hz
	const float lfoCfreq = 0.3111f; //hz
	const float lfoDfreq = 0.3222f; //hz

	const std::vector<int> chorusATaps{ 61,113,233,457,907,1733,3373,6569 };
	const std::vector<int> chorusBTaps{ 67,127,257,509,1021,2053,4091,8179 };

	array<float, 16> chorusLfoPhases;
	array<float, 16> chorusLfoDeltap;
	array<EnvFollower, 16> envelopes;

	Ran rand;

	StateVariableFilter LfilterA;
	StateVariableFilter LfilterB;

	StateVariableFilter RfilterA;
	StateVariableFilter RfilterB;

	LerpLine<float, 211>  input1;
	DelayLine<float, 157> input2;
	LerpLine<float, 563>  input3;
	DelayLine<float, 179> input4;

	DelayLine<float, 997>  Ldecay1;
	DelayLine<float, 2663> Ldecay2;
	DelayLine<float, 1327> Ldecay3;
	DelayLine<float, 3931> Ldecay4;
	LerpLine<float, 6599>  Ltank1;
	LerpLine<float, 5519>  Ltank2;


	DelayLine<float, 1009> Rdecay1;
	DelayLine<float, 2647> Rdecay2;
	DelayLine<float, 1321> Rdecay3;
	DelayLine<float, 3967> Rdecay4;
	LerpLine<float, 6491>  Rtank1;
	LerpLine<float, 5689>  Rtank2;

	LerpLine<float, 8209> pan1;
	LerpLine<float, 9731> pan2;

	float lfoADp = 0.0f;
	float lfoBDp = 0.0f;
	float lfoCDp = 0.0f;
	float lfoDDp = 0.0f;

	float lfoAPhase = 0.0f;
	float lfoBPhase = 0.0f;
	float lfoCPhase = 0.0f;
	float lfoDPhase = 0.0f;

	float lfoA = 0.0f;
	float lfoB = 0.0f;
	float lfoC = 0.0f;
	float lfoD = 0.0f;
	float lfoE = 0.0f;

	float dt;


	//const float minCutoff = 20.0f;
	//const float maxCutoff = 10560.0f;


public:

	ReverbEffect(const float sampleRate_, shared_ptr<ModMatrix> matrix_, const int channel_) {
		sampleRate = sampleRate_;
		matrix = matrix_;
		channel = channel_;

		interface.options = {
			{ "Wet / Dry Mix", 0.0f, 1.0f,        nullptr, 1 },
			{ "Chorus Level", 0.0f, 1.0f,         nullptr, 1 },
			{ "Response", 0.0f, 1.0f,             nullptr, 1 },
			{ "balance", 0.0f, 1.0f,              nullptr, 1 },
			{ "diffusion", 0.0f, 0.5f,            nullptr, 1 },
			{ "depth", 0.0f, 1.0f,                nullptr, 1 },
			{ "rate", 0.0f, 10.0f,                nullptr, 1 },
			{ "damping", 0.0f, 1.0f / sqrt(2.0f), nullptr, 1 }
		};

		registerInterface();

		*interface.options[1].resP = 0.5f;
		*interface.options[2].resP = 0.5f;
		*interface.options[4].resP = 0.32f;
		*interface.options[6].resP = 1.0f;
		*interface.options[7].resP = 1.0f / sqrt(2.0f);

		dt = 1.0f / sampleRate_;
		lfoADp = (lfoAfreq / sampleRate_) * dt;
		lfoBDp = (lfoBfreq / sampleRate_) * dt;
		lfoCDp = (lfoCfreq / sampleRate_) * dt;
		lfoDDp = (lfoDfreq / sampleRate_) * dt;

		for (auto& f : envelopes) {
			f.zero(sampleRate_);
			f.setFreq(10.0f + (10.0f * rand.flt()));
		}

		zero();
	}

	void zero() {
		input1.zero();
		input2.zero();
		input3.zero();
		input4.zero();

		Ldecay1.zero();
		Ldecay2.zero();
		Ldecay3.zero();
		Ldecay4.zero();

		Ltank1.zero();
		Ltank2.zero();

		Rdecay1.zero();
		Rdecay2.zero();
		Rdecay3.zero();
		Rdecay4.zero();

		Rtank1.zero();
		Rtank2.zero();

		LfilterA.zero(sampleRate);
		LfilterB.zero(sampleRate);

		LfilterA.setFreq(603.0f);
		LfilterB.setFreq(553.0f);

		RfilterA.zero(sampleRate);
		RfilterB.zero(sampleRate);

		RfilterA.setFreq(603.0f);
		RfilterB.setFreq(553.0f);

		for (auto& f : envelopes) {
			f.zero(sampleRate);
			f.setFreq(10.0f + (10.0f * rand.flt()));
		}

		chorusLfoDeltap.fill(0.0f);
		chorusLfoPhases.fill(0.0f);

	}


	//0-1
	void setTone(const float t_) {
		const float baseFreq = 190.0f * powf(1796.0f / 190.0f, t_);
		const float f2 = 12.0f / 15.0f * baseFreq;
		const float f3 = 5.0f / 6.0f * f2;

		LfilterA.setFreq(baseFreq);
		LfilterB.setFreq(f2);
		RfilterA.setFreq(f3);

	}

	void processLFOs() {
		lfoAPhase += lfoADp;
		lfoBPhase += lfoBDp;
		lfoCPhase += lfoCDp;
		lfoDPhase += lfoDDp;

		if (lfoAPhase > 1.0f) {
			lfoAPhase -= 1.0f;
		}

		if (lfoBPhase > 1.0f) {
			lfoBPhase -= 1.0f;
		}

		if (lfoCPhase > 1.0f) {
			lfoCPhase -= 1.0f;
		}

		if (lfoDPhase > 1.0f) {
			lfoDPhase -= 1.0f;
		}

		lfoA = aproxSine(2.0f * PI * lfoAPhase);
		lfoB = aproxSine(2.0f * PI * lfoBPhase);
		lfoC = aproxSine(2.0f * PI * lfoCPhase);
		lfoD = aproxSine(2.0f * PI * lfoDPhase);
		lfoE = aproxSine(2.0f * PI * lfoDPhase);


		//Chorus stuff
		for (int c = 0; c < 16; c++) {
			envelopes[c].process(rand.flt());
			chorusLfoPhases[c] += 0.0000000001f * *interface.options[6].resP * static_cast<float>(c * 10);  //0ish-1ish - rate multiplier perhaps

			if (chorusLfoPhases[c] > 1.0f) {
				chorusLfoPhases[c] -= 1.0f;
			}
		}


	}

	//, const float mix_, const float chorusMix_
	void process_audio(pair<float, float>& in_) {

		const auto mix       = *interface.options[0].resP;
		const auto chorusMix = *interface.options[1].resP;
		const auto response  = *interface.options[2].resP;
		const auto balance   = *interface.options[3].resP;
		const auto diffusion = *interface.options[4].resP;
		const auto depth     = *interface.options[5].resP;
		const auto rate      = *interface.options[6].resP;
		const auto damping   = *interface.options[7].resP;


		setTone(response);
		processLFOs();

		float accumulator_l = get<0>(in_) * panL(balance);
		float accumulator_r = get<1>(in_) * panR(balance);
		assert(isnormal(accumulator_l));
		assert(isnormal(accumulator_r));
		//Smear input
		input1.set(100, input1.tap(0.6f + 0.4f * lfoA));
		input3.set(100, input3.tap(0.6f + 0.4f * lfoA));

		accumulator_l += input1.tap(1.0f) * diffusion;
		input1.set(accumulator_l * -diffusion);

		accumulator_l += input2.get() * diffusion;
		input2.set(accumulator_l * -diffusion);



		accumulator_r += input3.tap(1.0f) * diffusion;
		input3.set(accumulator_r * -diffusion);
		accumulator_r += input4.get() * diffusion;
		input4.set(accumulator_r * -diffusion);

		assert(isnormal(accumulator_l));
		assert(isnormal(accumulator_r));

		//Left 
		//Left side

		accumulator_l += Ltank2.tap(0.6f + (0.4f * lfoB)) * damping;
		LfilterA.process(accumulator_l);
		accumulator_l = LfilterA.lowpass();

		accumulator_l += Ldecay1.get() * -diffusion;
		Ldecay1.set(accumulator_l * diffusion);

		accumulator_l += Ldecay2.get() * diffusion;
		Ldecay2.set(accumulator_l * -diffusion);

		assert(isnormal(accumulator_l));

		Ltank1.set(accumulator_l);

		assert(isnormal(accumulator_l));

		//Chorus up tank1
		float chorus = 0.0f;
		const float Ltank1Scale = 1.0f / static_cast<float>(Ltank1.size());
		for (int t = 0; t < chorusATaps.size(); t++) {
			const float tapScale = static_cast<float>(chorusATaps[t]) * Ltank1Scale;
			const float amMod = 1.0f - envelopes[t].envelope();

			chorus += Ltank1.tap(clamp(tapScale * (depth * aproxSine(2.0f * PI * chorusLfoPhases[t] + 1.0f) / 2.0f), 0.0f, 1.0f)) * 0.6f * amMod;
			assert(isnormal(chorus));
		}
		assert(isnormal(chorus));
		assert(isnormal(accumulator_l));

		const float subMix1 = lerp<float>(accumulator_l, chorus, chorusMix);
		get<0>(in_) = crossfade<float>(get<0>(in_), subMix1, mix);


		//Right Side

		accumulator_r += Ltank1.tap(0.6f + (0.4f * lfoC)) * damping;
		//accumulator += lerp(Ltank1.tap(0.6f + (0.4f * lfoC)), chorus, chorusMix_) * damping;
		LfilterB.process(accumulator_r);
		accumulator_r = LfilterB.lowpass();

		accumulator_r += Ldecay3.get() * diffusion;
		Ldecay3.set(accumulator_r * -diffusion);

		accumulator_r += Ldecay4.get() * -diffusion;
		Ldecay4.set(accumulator_r * diffusion);

		assert(isnormal(accumulator_r));

		pan1.set(accumulator_r);

		assert(isnormal(accumulator_r));

		//Chorus up pan1
		chorus = 0.0f;
		const float pan1Scale = 1.0f / static_cast<float>(pan1.size());
		for (int t = 0; t < chorusBTaps.size(); t++) {
			const float env = envelopes[8 + (size_t)t].envelope();

			const float tapScale = static_cast<float>(chorusBTaps[t]) * pan1Scale;
			const float amMod = 1.0f - env;

			chorus += pan1.tap(clamp(tapScale * (depth * aproxSine(2.0f * PI * chorusLfoPhases[8 + (size_t)t] + 1.0f) / 2.0f), 0.0f, 1.0f)) * 0.6f * amMod;
		}

		assert(isnormal(chorus));
		assert(isnormal(accumulator_r));

		const float subMix2 = lerp<float>(accumulator_r, chorus, chorusMix);
		get<1>(in_) = crossfade<float>(get<1>(in_), subMix2, mix);

		//New 3rd section
		accumulator_r = pan1.tap(0.6f + (0.4f * lfoD)) * damping;
		//accumulator += lerp(pan1.tap(0.6f + (0.4f * lfoD)), chorus, chorusMix_) * damping;
		RfilterA.process(accumulator_r);
		accumulator_r = RfilterA.lowpass();

		accumulator_r += Rdecay1.get() * -diffusion;
		Rdecay1.set(accumulator_r * diffusion);

		accumulator_r += Rdecay2.get() * diffusion;
		Rdecay2.set(accumulator_r * -diffusion);

		Ltank2.set(tanh(accumulator_r));

	}

};
*/


class Spatializer {
private:
	float behind_;
	float left_;
	float right_;
	float angle_;
	float distance_;
	float fixed_position_;

	StateVariableFilter filter;

public:
	Spatializer() { }
	~Spatializer() { }

	void Init(const float fixed_position, const float sample_rate_) {
		angle_ = 0.0f;
		fixed_position_ = fixed_position;
		left_ = 0.0f;
		right_ = 0.0f;
		distance_ = 0.0f;
		filter.zero(sample_rate_);
		filter.Q = 1.0f;
		filter.setFreq(1000.0f);
	}

	inline void Rotate(float rotation_speed) {
		angle_ += rotation_speed;
		if (angle_ >= 1.0f) {
			angle_ -= 1.0f;
		}
		if (angle_ < 0.0f) {
			angle_ += 1.0f;
		}
	}

	inline void set_distance(float distance) {
		distance_ = distance;
	}

	void Process(float* source, float* center, float* sides, size_t size) {
		filter.process(*source);
		behind_ = filter.lowpass();

		const float angle = angle_;
		float x = cosf(2.0f * PI * angle);
		const float y = sinf(2.0f * PI * angle);
		const float backfront = (1.0f + y) * 0.5f * distance_;
		x += fixed_position_ * (1.0f - distance_);

		const float target_left = cosf(2.0f * PI * ((1.0f + x) * 0.125f));
		const float target_right = sinf(2.0f * PI * ((3.0f + x) * 0.125f));

		// Prevent zipper noise during rendering.
		const float left_increment = (target_left - left_);
		const float right_increment = (target_right - right_);

		left_ += left_increment;
		right_ += right_increment;
		const float z = *source + backfront * (behind_ - *source);
		const float l = left_ * z;
		const float r = right_ * z;
		*center = (l + r) * 0.5f;
		*sides = (l - r) * 0.5f / 0.7f;
	}



};



//I couldn't help myself
struct ReverbEffect : AudioUnit {
private:
	elements::Reverb reverb;
	vector<uint16_t> reverb_buffer;
	pair<float, float> last;
	pair<float, float> center;
	pair<float, float> side;
	array<Spatializer,2> spatializers;
	
	float dt;

public:

	ReverbEffect(const float sampleRate_, shared_ptr<ModMatrix> matrix_, const int channel_) {
		sampleRate = sampleRate_;
		matrix = matrix_;
		channel = channel_;

		reverb_buffer.resize(32768,0);
		memset(&reverb, 0, sizeof(reverb));
		reverb.Init(reverb_buffer.data());
		last = make_pair(0.0f, 0.0f);
		center = make_pair(0.0f, 0.0f);
		side = make_pair(0.0f, 0.0f);

		//memset(&spatializers, 0, sizeof(spatializers));
		spatializers[0].Init(-0.5f, sampleRate_);
		spatializers[1].Init(0.5f, sampleRate_);


		interface.options = {
			{ "Wet / Dry Mix", 0.0f, 1.0f,  nullptr, 1 },
			{ "Space", 0.0f, 1.0f,          nullptr, 1 },
			{ "Feedback", 0.0f, 1.0f,       nullptr, 1 },
			{ "Feedback Swirl", 0.0f, 1.0f, nullptr, 1 }
		};

		registerInterface();

		*interface.options[0].resP = 0.5f;
		*interface.options[1].resP = 0.5f;
		*interface.options[2].resP = 0.0f;
		*interface.options[3].resP = 0.32f;

		dt = 1.0f / sampleRate_;

	}

	//, const float mix_, const float chorusMix_
	void process_audio(pair<float, float>& in_) {

		const auto mix      = *interface.options[0].resP;
		const auto space    = *interface.options[1].resP;
		const auto fb       = *interface.options[2].resP;
		const auto fb_swirl = *interface.options[3].resP;

		spatializers[0].Rotate(fb_swirl * 0.001f);
		spatializers[0].set_distance(space * (2.0f - space));
		spatializers[0].Process(&get<0>(last), &get<0>(center), &get<0>(side), 1);
		spatializers[1].Rotate(fb_swirl * 0.001f);
		spatializers[1].set_distance(space * (2.0f - space));
		spatializers[1].Process(&get<1>(last), &get<1>(center), &get<1>(side), 1);
		
		const float spread = space <= 0.7f ? space : 0.7f;
		const float sl = get<0>(side) * spread;
		const float sr = get<1>(side) * spread;

		float lr = get<0>(center) - sl;
		float ll = get<0>(center) + sl;

		float rr = get<1>(center) - sr;
		float rl = get<1>(center) + sr;
		
		get<0>(last) = (ll + rl) * 0.5f;
		get<1>(last) = (lr + rr) * 0.5f;

		get<0>(in_) = lerp<float>(get<0>(in_), get<0>(in_) + get<0>(last) * 0.5f, fb);
		get<1>(in_) = lerp<float>(get<1>(in_), get<1>(in_) + get<1>(last) * 0.5f, fb);

		//get<0>(in_) = get<0>(in_) + get<0>(last) * 0.5f;
		//get<1>(in_) = get<1>(in_) + get<1>(last) * 0.5f;

		float reverb_amount  = space * 0.95f;
		reverb_amount += fb * (2.0f - fb);
		CONSTRAIN(reverb_amount, 0.0f, 1.0f);

		reverb.set_amount(reverb_amount * 0.54f);
		reverb.set_diffusion(0.7f);
		reverb.set_time(0.35f + 0.63f * reverb_amount);
		reverb.set_input_gain(0.2f);
		reverb.set_lp(0.6f + 0.37f * fb);
		reverb.Process(&get<0>(in_), &get<1>(in_), 1);

		last = in_;
	}

};



































struct GranularEffect : AudioUnit {
private:
	clouds::PlaybackMode mode;
	clouds::GranularProcessor* processor;
	//array<uint8_t, 118784> block_mem;
	//array<uint8_t, 65536 - 128> block_ccm;
	uint8_t* block_mem = nullptr;
	uint8_t* block_ccm = nullptr;
	//array<Trigger, POLY> triggers;
	//array<Trigger, POLY> freeze_triggers;
	Trigger trigger;
	Trigger freeze_trigger;
	clouds::ShortFrame input[1] = {};
	clouds::ShortFrame output[1] = {};


	Kit* channel_kit;

	int current_action_gate = 0;
	int current_freeze_gate = 0;

public:

	GranularEffect(const float sampleRate_, shared_ptr<ModMatrix> matrix_, const int channel_, Kit* channel_kit_) {
		sampleRate = sampleRate_;
		matrix = matrix_;
		channel = channel_;
		channel_kit = channel_kit_;

		mode = clouds::PLAYBACK_MODE_GRANULAR;

		processor = new clouds::GranularProcessor();
		memset(processor, 0, sizeof(*processor));

		

		//block_mem.fill(0);
		//block_ccm.fill(0);

		block_mem = new uint8_t[118784]();
		memset(block_mem, 0, sizeof(*block_mem));

		block_ccm = new uint8_t[65536 - 128]();
		memset(block_ccm, 0, sizeof(*block_ccm));

		//processor->Init(block_mem.data(), block_mem.size(), block_ccm.data(), block_ccm.size());
		processor->Init(block_mem, 118784, block_ccm, 65536 - 128);
		processor->set_num_channels(2);
		processor->set_low_fidelity(false); // can't use lofi mode with input/output length of 1
		processor->set_playback_mode(mode);
		processor->Prepare();

		current_action_gate = 0;
		current_freeze_gate = 0;

		//interface.options = {
		//	{ "Mix",      0.0f, 1.0f, nullptr, 1 },
		//	{ "Overlap",  0.0f, 1.0f, nullptr, 1 },
		//	{ "Window",   0.0f, 1.0f, nullptr, 1 },
		//	{ "GSpread",  0.0f, 1.0f, nullptr, 1 },
		//};
		//
		//interface.consumers = {
		//	{ "Action",   0.0f, 1.0f, nullptr, POLY },
		//	{ "Freeze",   0.0f, 1.0f, nullptr, POLY },
		//	{ "Position", 0.0f, 1.0f, nullptr, POLY },
		//	{ "Size",     0.0f, 1.0f, nullptr, POLY },
		//	{ "Density",  0.0f, 1.0f, nullptr, POLY },
		//	{ "Texture",  0.0f, 1.0f, nullptr, POLY },
		//	{ "Spread",   0.0f, 1.0f, nullptr, POLY },
		//	{ "Feedback", 0.0f, 1.0f, nullptr, POLY },
		//	{ "Reverb",   0.0f, 1.0f, nullptr, POLY }
		//};
		//
		//registerInterface();
		//
		//
		//for (int i = 0; i < POLY; i++) {
		//	interface.consumers[0].resP[i] = 0.0f;
		//	interface.consumers[1].resP[i] = 0.0f;
		//	interface.consumers[2].resP[i] = 0.0f;
		//	interface.consumers[3].resP[i] = 0.0f;
		//	interface.consumers[4].resP[i] = 0.5f;
		//	interface.consumers[5].resP[i] = 0.0f;
		//	interface.consumers[6].resP[i] = 0.0f;
		//	interface.consumers[7].resP[i] = 0.0f;
		//	interface.consumers[8].resP[i] = 0.5f;
		//}

		interface.options = {
			{ "Mix",      0.0f, 1.0f, nullptr, 1 },
			{ "Overlap",  0.0f, 1.0f, nullptr, 1 },
			{ "Window",   0.0f, 1.0f, nullptr, 1 },
			{ "GSpread",  0.0f, 1.0f, nullptr, 1 },
			{ "Action",   0.0f, 1.0f, nullptr, 1 },
			{ "Freeze",   0.0f, 1.0f, nullptr, 1 },
			{ "Position", 0.0f, 1.0f, nullptr, 1 },
			{ "Size",     0.0f, 1.0f, nullptr, 1 },
			{ "Density",  0.0f, 1.0f, nullptr, 1 },
			{ "Texture",  0.0f, 1.0f, nullptr, 1 },
			{ "Spread",   0.0f, 1.0f, nullptr, 1 },
			{ "Feedback", 0.0f, 1.0f, nullptr, 1 },
			{ "Reverb",   0.0f, 1.0f, nullptr, 1 }
		};

		registerInterface();

	}

	~GranularEffect() {
		free(block_mem);
		block_mem = nullptr;
		free(block_ccm);
		block_ccm = nullptr;
	}

	void process_audio(pair<float, float>& in_) {
		//const auto action_in   = interface.consumers[0].resP;
		//const auto freeze_in   = interface.consumers[1].resP;
		//const auto position_in = interface.consumers[2].resP;
		//const auto size_in     = interface.consumers[3].resP;
		//const auto density_in  = interface.consumers[4].resP;
		//const auto texture_in  = interface.consumers[4].resP;
		//const auto spread_in   = interface.consumers[5].resP;
		//const auto fb_in       = interface.consumers[6].resP;
		//const auto reverb_in   = interface.consumers[7].resP;
		//
		//const auto mix         = *interface.options[0].resP;
		//const auto overlap     = *interface.options[1].resP;
		//const auto window      = *interface.options[2].resP;
		//const auto gspread     = *interface.options[3].resP;
		//
		//bool triggered = false;
		//
		//for (int i = 0; i < POLY; i++) {
		//	if (triggers[i].process(action_in[i])) {
		//		current_action_gate = i;
		//		//patch.note = 
		//		triggered = true;
		//	}
		//
		//	if (freeze_triggers[i].process(freeze_in[i])) {
		//		current_freeze_gate = i;
		//	}
		//
		//}
		//
		//clouds::Parameters* p     = processor->mutable_parameters();
		//p->trigger                = triggered;
		//p->gate                   = action_in[current_action_gate];
		//p->freeze                 = freeze_in[current_freeze_gate];
		//p->position               = position_in[current_action_gate];
		//p->size                   = size_in[current_action_gate];
		//p->pitch                  = (float)channel_kit->notes[current_action_gate].n; //yes it is dammit
		//p->density                = density_in[current_action_gate];
		//p->texture                = texture_in[current_action_gate];
		//p->dry_wet                = mix;
		//p->stereo_spread          = spread_in[current_action_gate];
		//p->feedback               = fb_in[current_action_gate];
		//p->reverb                 = reverb_in[current_action_gate];
		//p->granular.overlap       = overlap;
		//p->granular.window_shape  = window;
		//p->granular.stereo_spread = gspread;

		const auto mix      = *interface.options[0].resP;
		const auto overlap  = *interface.options[1].resP;
		const auto window   = *interface.options[2].resP;
		const auto gspread  = *interface.options[3].resP;
		const auto action   = *interface.options[4].resP;
		const auto freeze   = *interface.options[5].resP;
		const auto position = *interface.options[6].resP;
		const auto size     = *interface.options[7].resP;
		const auto density  = *interface.options[8].resP;
		const auto texture  = *interface.options[9].resP;
		const auto spread   = *interface.options[10].resP;
		const auto fb       = *interface.options[11].resP;
		const auto reverb   = *interface.options[12].resP;

		
		clouds::Parameters* p     = processor->mutable_parameters();
		p->trigger                = trigger.process(action);
		p->gate                   = p->trigger;
		p->freeze                 = freeze_trigger.process(freeze);
		p->position               = position;
		p->size                   = size;
		p->pitch                  = channel_kit->notes[0].n;
		p->density                = density;
		p->texture                = texture;
		p->dry_wet                = mix;
		p->stereo_spread          = spread;
		p->feedback               = fb;
		p->reverb                 = reverb;
		p->granular.overlap       = overlap;
		p->granular.window_shape  = window;
		p->granular.stereo_spread = gspread;

		
		input[0].l = get<0>(in_) * 32767;
		input[0].r = get<1>(in_) * 32767;

		processor->Process(input, output, 1);

		in_ = make_pair(output->l * (1.0f / 32768.0f), output->r * (1.0f / 32768.0f));

	}

};


























































/*
struct ReverbEffect : AudioUnit {
private:

	const float min_response = 40.0;
	const float max_response = 7350.0;


	const std::vector<int> chorus_taps{ 61,113,233,457,907,1733,3373,5987 };

	//16304
	array<AllPassFilter, 10> allpass_l;
	array<AllPassFilter, 10> allpass_r;

	LerpLine<float, 24576>  left;
	LerpLine<float, 24576>  right;
	PitchShifter ps;

	array<float, 12> lfo_phase;
	array<float, 12> lfo_dp;
	array<EnvFollower, 8> envelopes;

	Ran rand;

	float dt = 0.0f;
	float mix = 0.0f;
	float chorusMix = 0.0f;
	float response = 0.0f;
	float balance = 0.5f;
	float diffusion = 0.0f; // 0-1
	float feedback = 0.0f; // 0-1 ... careful!
	float depth = 0.0f; // 0-1
	float rate = 1.0f; //1  - 10 or 20 perhaps
	float damping = 1.0f / sqrt(2.0f);

	inline float lfo(const float p_) {
		return (sin(2.0f * PI * p_) + 1.0f) * 0.5f;
	}


public:

	ReverbEffect(const float sampleRate_, shared_ptr<ModMatrix> matrix_, const int channel_) {
		sampleRate = sampleRate_;
		matrix = matrix_;
		channel = channel_;

		dt = 1.0f / sampleRate_;
		mix = 0.0f;
		response = 0.5f;
		balance = 0.5f;
		diffusion = 0.4f; // 0-1
		feedback = 0.0f; // 0-1 ... careful!
		depth = 0.0f; // 0-1
		rate = 1.0f; //1  - 10 or 20 perhaps
		damping = 1.0f / sqrt(2.0f);

		interface.options = {
			{ "Wet / Dry Mix", 0.0f, 1.0f, &mix, 1 },
			{ "Response", 0.0f, 1.0f, &response, 1 },
			{ "balance", 0.0f, 1.0f, &balance, 1 },
			{ "diffusion", 0.0f, 0.5f, &diffusion, 1 },
			{ "feedback", 0.0f, 1.0f, &feedback, 1 },
			{ "depth", 0.0f, 1.0f, &depth, 1 },
			{ "rate", 0.0f, 10.0f, &rate, 1 },
			{ "damping", 0.0f, 1.0f / sqrt(2.0f), &damping, 1 }

		};

		for (auto& ap : allpass_l) {
			ap.setResponse(0.0f);
		}

		for (auto& ap : allpass_r) {
			ap.setResponse(0.0f);
		}

		left.zero();
		right.zero();

		lfo_phase.fill(0.0f);
		lfo_dp.fill(0.0f);

		lfo_dp[0] = 0.2021f * dt; //left swizzle
		lfo_dp[1] = 0.2303f * dt; //right swizzle


		for (auto& f : envelopes) {
			f.zero(sampleRate_);
			f.setFreq(10.0f + (10.0f * rand.flt()));
		}

		ps.zero();

	}

	void processLFOs() {

		//lfo_dp[0] alreday set
		//lfo_dp[1]
		lfo_dp[2] = rate * dt;
		lfo_dp[3] = rate * dt;

		//Chorus stuff
		for (int c = 0; c < 8; c++) {
			envelopes[c].smoothness(depth);
			envelopes[c].process(rand.flt());
			lfo_dp[4+c] = 0.0000000001f * rate * static_cast<float>(c * 10);  //0ish-1ish - rate multiplier perhaps
		}

		for (int i = 0; i < 12; i++) {
			lfo_phase[i] += lfo_dp[i];
			if (lfo_phase[i] > 1.0f) {
				lfo_phase[i] -= 1.0f;
			}
		}


	}

	//, const float mix_, const float chorusMix_
	void process_audio(pair<float, float>& in_) {
		processLFOs();
		//440.0f, 1600.0f
		const float ml = 440.0f + 1200.0f * lfo(lfo_phase[0]);
		const float mr = 440.0f + 1200.0f * lfo(lfo_phase[1]);
		const float diff = diffusion;

		for (int i = 0; i < 4; i++) {
			allpass_l[i].setResponse(ml);
		}

		for (int i = 4; i < 10; i++) {
			allpass_l[i].setResponse(diff);
		}

		ps.set(make_pair(right.tap(1.0f) * feedback * panL(balance), left.tap(1.0f) * feedback * panR(balance)));
		auto fb = ps.shifted(0.8f);

		left.set(allpass_l[0].process(
			allpass_l[1].process(
				allpass_l[2].process(
					allpass_l[3].process(
						allpass_l[4].process(
							allpass_l[5].process(get<0>(in_) + get<0>(fb)))))))
		);


		//smear input
		//float smear = 100.0f * depth * lfo(lfo_phase[2]);
		//left.set(100, left.tap(100 + static_cast<int>(smear), smear - static_cast<int>(smear)));

		//left.write(997, allpass_l[6].process(left.tap(6323)));
		//left.write(6323, -allpass_l[7].process(left.tap(7650)));
		//left.write(7650, allpass_l[8].process(left.tap(11581)));
		//left.write(11581, -allpass_l[9].process(left.tap(18180)));

		//smear = 6599.0f * depth * lfo(lfo_phase[2]);
		//left.write(18180, left.tap(18180 + static_cast<int>(smear), smear - static_cast<int>(smear)) * damping);
		//smear = 5519.0f * depth * lfo(lfo_phase[2]);
		//left.write(23699, left.tap(23699 + static_cast<int>(smear), smear - static_cast<int>(smear)) * damping);

		//DelayLine<float, 997>  Ldecay1;
		//DelayLine<float, 2663> Ldecay2; 6323
		//DelayLine<float, 1327> Ldecay3; 7650
		//DelayLine<float, 3931> Ldecay4; 11581
		//LerpLine<float, 6599>  Ltank1;  18180
		//LerpLine<float, 5519>  Ltank2;  23699

		//
		// Right side
		//

		for (int i = 0; i < 4; i++) {
			allpass_r[i].setResponse(mr);
		}

		for (int i = 4; i < 10; i++) {
			allpass_r[i].setResponse(diff);
		}

		right.set(allpass_r[0].process(
			allpass_r[1].process(
				allpass_r[2].process(
					allpass_r[3].process(
						allpass_r[4].process(
							allpass_r[5].process(get<1>(in_) + get<1>(fb)))))))
		);

		//smear input
		//smear = 100.0f * depth * lfo(lfo_phase[3]);
		//right.set(100, right.tap(100 + static_cast<int>(smear), smear - static_cast<int>(smear)));

		//right.write(1009, allpass_r[6].process(right.tap(3656)));
		//right.write(3656, -allpass_r[7].process(right.tap(4977)));
		//right.write(4977, allpass_r[8].process(right.tap(8944)));
		//right.write(8944, -allpass_r[9].process(right.tap(15435)));

		//smear = 6599.0f * depth * lfo(lfo_phase[3]);
		//right.write(15435, right.tap(15435 + static_cast<int>(smear), smear - static_cast<int>(smear)) * damping);
		//smear = 5519.0f * depth * lfo(lfo_phase[3]);
		//right.write(21124, right.tap(21124 + static_cast<int>(smear), smear - static_cast<int>(smear)) * damping);

		//DelayLine<float, 1009> Rdecay1;
		//DelayLine<float, 2647> Rdecay2; 3656
		//DelayLine<float, 1321> Rdecay3; 4977
		//DelayLine<float, 3967> Rdecay4; 8944
		//LerpLine<float, 6491>  Rtank1; 15435
		//LerpLine<float, 5689>  Rtank2; 21124


		//l tank1 11581 - 18180 len 6599
		//r tank1 8944 - 15435 len 6491
		float chorus_l = 0.0f;
		float chorus_r = 0.0f;
		const float scale_l = 1.0f / static_cast<float>(6599);
		const float scale_r = 1.0f / static_cast<float>(6491);
		for (int t = 0; t < chorus_taps.size(); t++) {
			const float env = 1.0f - envelopes[t].envelope();
			const float idx = chorus_taps[t] * env;
			chorus_l += left.tap(18180 + static_cast<int>(idx), idx - static_cast<int>(idx));
			chorus_r += right.tap(15435 + static_cast<int>(idx), idx - static_cast<int>(idx));

			//assert(isnan(chorus_l));
			//assert(isnan(chorus_r));
		}

		get<0>(in_) = crossfade<float>(get<0>(in_), chorus_l, mix);
		
		get<1>(in_) = crossfade<float>(get<1>(in_), chorus_r, mix);



	}

};

*/





























/*
//Chunky but working
struct ReverbEffect : AudioUnit {
private:
	
	const float minCutoff = 40.0;
	const float maxCutoff = 7350.0;
	const float lfoAfreq = 0.2f; //hz
	const float lfoBfreq = 0.2333f; //hz
	const float lfoCfreq = 0.3111f; //hz
	const float lfoDfreq = 0.3222f; //hz

	const std::vector<int> chorusATaps{ 61,113,233,457,907,1733,3373,6569 };
	const std::vector<int> chorusBTaps{ 67,127,257,509,1021,2053,4091,8179 };

	array<float, 16> chorusLfoPhases;
	array<float, 16> chorusLfoDeltap;
	array<EnvFollower, 16> envelopes;
	
	Ran rand;

	StateVariableFilter LfilterA;
	StateVariableFilter LfilterB;

	StateVariableFilter RfilterA;
	StateVariableFilter RfilterB;

	LerpLine<float, 211>  input1;
	DelayLine<float, 157> input2;
	LerpLine<float, 563>  input3;
	DelayLine<float, 179> input4;

	DelayLine<float, 997>  Ldecay1;
	DelayLine<float, 2663> Ldecay2;
	DelayLine<float, 1327> Ldecay3;
	DelayLine<float, 3931> Ldecay4;
	LerpLine<float, 6599>  Ltank1;
	LerpLine<float, 5519>  Ltank2;


	DelayLine<float, 1009> Rdecay1;
	DelayLine<float, 2647> Rdecay2;
	DelayLine<float, 1321> Rdecay3;
	DelayLine<float, 3967> Rdecay4;
	LerpLine<float, 6491>  Rtank1;
	LerpLine<float, 5689>  Rtank2;

	LerpLine<float, 8209> pan1;
	LerpLine<float, 9731> pan2;

	float lfoADp = 0.0f;
	float lfoBDp = 0.0f;
	float lfoCDp = 0.0f;
	float lfoDDp = 0.0f;

	float lfoAPhase = 0.0f;
	float lfoBPhase = 0.0f;
	float lfoCPhase = 0.0f;
	float lfoDPhase = 0.0f;

	float lfoA = 0.0f;
	float lfoB = 0.0f;
	float lfoC = 0.0f;
	float lfoD = 0.0f;
	float lfoE = 0.0f;

	float dt;

	
	//const float minCutoff = 20.0f;
	//const float maxCutoff = 10560.0f;

	float mix = 0.0f;
	float chorusMix = 0.0f;
	float response = 0.0f;
	float balance = 0.5f;
	float diffusion = 0.0f; // 0-1
	//float decay = 0.0f;
	float feedback = 0.0f; // 0-1 ... careful!
	float depth = 0.0f; // 0-1
	float rate = 1.0f; //1  - 10 or 20 perhaps
	float damping = 1.0f / sqrt(2.0f);


public:

	ReverbEffect(const float sampleRate_, shared_ptr<ModMatrix> matrix_, const int channel_) {
		sampleRate = sampleRate_;
		matrix = matrix_;
		channel = channel_;

		mix = 0.0f;
		chorusMix = 0.5f;
		response = 0.5f;
		balance = 0.5f;
		diffusion = 0.4f; // 0-1
		feedback = 0.0f; // 0-1 ... careful!
		depth = 0.0f; // 0-1
		rate = 1.0f; //1  - 10 or 20 perhaps
		damping = 1.0f / sqrt(2.0f);

		interface.options = {
			{ "Wet / Dry Mix", 0.0f, 1.0f, &mix, 1 },
			{ "Chorus Level", 0.0f, 1.0f, &chorusMix, 1 },
			{ "Response", 0.0f, 1.0f, &response, 1 },
			{ "balance", 0.0f, 1.0f, &balance, 1 },
			{ "diffusion", 0.0f, 0.5f, &diffusion, 1 },
			{ "depth", 0.0f, 1.0f, &depth, 1 },
			{ "rate", 0.0f, 10.0f, &rate, 1 },
			{ "damping", 0.0f, 1.0f / sqrt(2.0f), &damping, 1 }

		};


		dt = 1.0f / sampleRate_;
		lfoADp = (lfoAfreq / sampleRate_) * dt;
		lfoBDp = (lfoBfreq / sampleRate_) * dt;
		lfoCDp = (lfoCfreq / sampleRate_) * dt;
		lfoDDp = (lfoDfreq / sampleRate_) * dt;

		for (auto& f : envelopes) {
			f.zero(sampleRate_);
			f.setFreq(10.0f + (10.0f * rand.flt()));
		}

		chorusLfoPhases.fill(0.0f);
		chorusLfoDeltap.fill(0.0f);

		zero();
	}

	void zero() {
		input1.zero();
		input2.zero();
		input3.zero();
		input4.zero();

		Ldecay1.zero();
		Ldecay2.zero();
		Ldecay3.zero();
		Ldecay4.zero();

		Ltank1.zero();
		Ltank2.zero();

		Rdecay1.zero();
		Rdecay2.zero();
		Rdecay3.zero();
		Rdecay4.zero();

		Rtank1.zero();
		Rtank2.zero();

		LfilterA.zero(sampleRate);
		LfilterB.zero(sampleRate);

		LfilterA.setFreq(603.0f);
		LfilterB.setFreq(553.0f);

		RfilterA.zero(sampleRate);
		RfilterB.zero(sampleRate);

		RfilterA.setFreq(603.0f);
		RfilterB.setFreq(553.0f);

		//decay = 0.95f;
		diffusion = 0.625f;
		feedback = 0.0f;

		balance = 0.5f;

		for (auto& f : envelopes) {
			f.zero(sampleRate);
			f.setFreq(10.0f + (10.0f * rand.flt()));
		}

		chorusLfoDeltap.fill(0.0f);
		chorusLfoPhases.fill(0.0f);

	}


	//0-1
	void setTone(const float t_) {
		const float baseFreq = 190.0f * powf(1796.0f / 190.0f, t_);
		const float f2 = 12.0f / 15.0f * baseFreq;
		const float f3 = 5.0f / 6.0f * f2;

		LfilterA.setFreq(baseFreq);
		LfilterB.setFreq(f2);
		RfilterA.setFreq(f3);

	}

	void processLFOs() {
		lfoAPhase += lfoADp;
		lfoBPhase += lfoBDp;
		lfoCPhase += lfoCDp;
		lfoDPhase += lfoDDp;

		if (lfoAPhase > 1.0f) {
			lfoAPhase -= 1.0f;
		}

		if (lfoBPhase > 1.0f) {
			lfoBPhase -= 1.0f;
		}

		if (lfoCPhase > 1.0f) {
			lfoCPhase -= 1.0f;
		}

		if (lfoDPhase > 1.0f) {
			lfoDPhase -= 1.0f;
		}

		lfoA = sinf(2 * M_PI * lfoAPhase);
		lfoB = sinf(2 * M_PI * lfoBPhase);
		lfoC = sinf(2 * M_PI * lfoCPhase);
		lfoD = sinf(2 * M_PI * lfoDPhase);
		lfoE = cosf(2 * M_PI * lfoDPhase);


		//Chorus stuff
		for (int c = 0; c < 16; c++) {
			envelopes[c].process(rand.flt());
			chorusLfoPhases[c] += 0.0000000001f * rate * static_cast<float>(c * 10);  //0ish-1ish - rate multiplier perhaps

			if (chorusLfoPhases[c] > 1.0f) {
				chorusLfoPhases[c] -= 1.0f;
			}
		}


	}

	//, const float mix_, const float chorusMix_
	void process_audio(pair<float, float>& in_) {
		setTone(response);
		processLFOs();

		float accumulator_l = get<0>(in_) * panL(balance);
		float accumulator_r = get<1>(in_) * panR(balance);
		assert(isnormal(accumulator_l));
		assert(isnormal(accumulator_r));
		//Smear input
		input1.set(100, input1.tap(0.6f + 0.4f * lfoA));
		input3.set(100, input3.tap(0.6f + 0.4f * lfoA));

		accumulator_l += input1.tap(1.0f) * diffusion;
		input1.set(accumulator_l * -diffusion);

		accumulator_l += input2.get() * diffusion;
		input2.set(accumulator_l * -diffusion);



		accumulator_r += input3.tap(1.0f) * diffusion;
		input3.set(accumulator_r * -diffusion);
		accumulator_r += input4.get() * diffusion;
		input4.set(accumulator_r * -diffusion);

		assert(isnormal(accumulator_l));
		assert(isnormal(accumulator_r));

		//Left 
		//Left side

		accumulator_l += Ltank2.tap(0.6f + (0.4f * lfoB)) * damping;
		LfilterA.process(accumulator_l);
		accumulator_l = LfilterA.lowpass();

		accumulator_l += Ldecay1.get() * -diffusion;
		Ldecay1.set(accumulator_l * diffusion);

		accumulator_l += Ldecay2.get() * diffusion;
		Ldecay2.set(accumulator_l * -diffusion);

		assert(isnormal(accumulator_l));

		Ltank1.set(accumulator_l);

		assert(isnormal(accumulator_l));

		//Chorus up tank1
		float chorus = 0.0f;
		const float Ltank1Scale = 1.0f / static_cast<float>(Ltank1.size());
		for (int t = 0; t < chorusATaps.size(); t++) {
			const float tapScale = static_cast<float>(chorusATaps[t]) * Ltank1Scale;
			const float amMod = 1.0f - envelopes[t].envelope();

			chorus += Ltank1.tap(clamp(tapScale * (depth * sinf(2 * M_PI * chorusLfoPhases[t] + 1.0f) / 2.0f), 0.0f, 1.0f)) * 0.6f * amMod;
			assert(isnormal(chorus));
		}
		assert(isnormal(chorus));
		assert(isnormal(accumulator_l));

		const float subMix1 = lerp<float>(accumulator_l, chorus, chorusMix);
		get<0>(in_) = crossfade<float>(get<0>(in_), subMix1, mix);


		//Right Side

		accumulator_r += Ltank1.tap(0.6f + (0.4f * lfoC)) * damping;
		//accumulator += lerp(Ltank1.tap(0.6f + (0.4f * lfoC)), chorus, chorusMix_) * damping;
		LfilterB.process(accumulator_r);
		accumulator_r = LfilterB.lowpass();

		accumulator_r += Ldecay3.get() * diffusion;
		Ldecay3.set(accumulator_r * -diffusion);

		accumulator_r += Ldecay4.get() * -diffusion;
		Ldecay4.set(accumulator_r * diffusion);

		assert(isnormal(accumulator_r));

		pan1.set(accumulator_r);

		assert(isnormal(accumulator_r));

		//Chorus up pan1
		chorus = 0.0f;
		const float pan1Scale = 1.0f / static_cast<float>(pan1.size());
		for (int t = 0; t < chorusBTaps.size(); t++) {
			const float env = envelopes[8 + t].envelope();

			const float tapScale = static_cast<float>(chorusBTaps[t]) * pan1Scale;
			const float amMod = 1.0f - env;

			chorus += pan1.tap(clamp(tapScale * (depth * sinf(2 * M_PI * chorusLfoPhases[8 + t] + 1.0f) / 2.0f), 0.0f, 1.0f)) * 0.6f * amMod;
		}

		assert(isnormal(chorus));
		assert(isnormal(accumulator_r));

		const float subMix2 = lerp<float>(accumulator_r, chorus, chorusMix);
		get<1>(in_) = crossfade<float>(get<1>(in_), subMix2, mix);

		//New 3rd section
		accumulator_r = pan1.tap(0.6f + (0.4f * lfoD)) * damping;
		//accumulator += lerp(pan1.tap(0.6f + (0.4f * lfoD)), chorus, chorusMix_) * damping;
		RfilterA.process(accumulator_r);
		accumulator_r = RfilterA.lowpass();

		accumulator_r += Rdecay1.get() * -diffusion;
		Rdecay1.set(accumulator_r * diffusion);

		accumulator_r += Rdecay2.get() * diffusion;
		Rdecay2.set(accumulator_r * -diffusion);

		Ltank2.set(tanh(accumulator_r));

	}

};

*/