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
#include "Utilities.h"

using namespace std;

struct FMWTableOsc {
private:
	const float min_cutoff = 20.0f;
	const float max_cutoff = 10560.0f;

	const array<float, 12> mod_ratios = { 1.0f, 2.0f, 2.25f, 2.3333f, 2.5f, 2.6666f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };
	
	StateVariableFilter filter;
	Ran rng;
	PinkNoise pink;

	float filter_type = 0.0f;

	float dt = 0.0f;
	
	float target_freq = 0.0f;
	float freq = 0.0f;
	float slew = 0.0f;

	float base_phase = 0.0f;
	float base_freq = 0.0f;
	float base_target_freq = 0.0f;
	float phase_locked_resonance_level = 1.0f;


	float carrier_phase = 0.0f;
	float carrier_last = 0.0f;
	
	//vector<float>* carrier_table;
	float mod_phase = 0.0f;
	float mod_last = 0.0f;
	//vector<float>* mod_table;


public:


	void init(const float sample_rate_ ) {
		//ds = 1.0f / (sample_rate_ / static_cast<float>(4160));
		dt = 1.0f / sample_rate_;
		base_phase = 0.0f;
		phase_locked_resonance_level = 1.0f;
		filter_type = 0.0f;
		carrier_phase = 0.0f;
		carrier_last = 0.0f;
		mod_phase = 0.0f;
		mod_last = 0.0f;
		slew = 1.0f;
		target_freq = 0.0f;
		base_target_freq = 0.0f;
		freq = 0.0f;

		InitializePinkNoise(&pink, 12);

		filter.zero(sample_rate_);
		filter.setFreq(min_cutoff);
	}



	//0-1
	void set_filter_cutoff(const float c_) {
		filter.setFreq(min_cutoff * powf(max_cutoff / min_cutoff, c_));
	}

	//0-1
	void set_filter_type(const float t_) {
		filter_type = t_;
	}

	//hz, 0-1, 0-1
	void set_freq(const float hz_, const float res_, const float slew_) {
		base_target_freq = hz_;
		phase_locked_resonance_level = 1.0f + (res_ * 7.0f);
		target_freq = hz_ * phase_locked_resonance_level;
		slew = slew_;
	}

    //  FM       _  _     
    // 1   2  2  2| 2|
    //  \ /   |  +- ||
    //        1  1  1|   
    //              +-
	//Just doing the feedbacky ones
	//via fb_a and fb_b parameters
	//


	//
	//More clear if I track phase and just use that
	//
	//0-1,0-1,0-1,0-1
	float wav(const float &ratio_, const float &mod_level_, const float &noise_level_, const float &fb_carrier_, const float &fb_mod_) {

		const auto mod_s     = interpvec<float>(tableA, abs(mod_phase) * 4159.0f);
		const auto carrier_s = interpvec<float>(tableB, abs(carrier_phase) * 4159.0f);
		const auto envelope  = interpvec<float>(table_hsin, base_phase * 4159.0f);
		const auto mod_ratio = interparray<float, 12>(mod_ratios, ratio_ * 11.0f);

		//f.process((GeneratePinkNoise(&pink, rand) + 1.0f) * 0.5f);

		freq += (target_freq - freq) * slew;
		base_freq += (base_target_freq - base_freq) * slew;

		base_phase += base_freq * dt;
		
		//not sure how to de-branch this nicely yet... fix up
		if (mod_level_ > 0.0f) {
			carrier_phase += freq * mod_level_ * mod_s * dt;
			
			float mf = freq * mod_ratio * dt;
			if (fb_carrier_ > 0.0f) 
				mf *= carrier_s * fb_carrier_;
		
			if (fb_mod_ > 0.0f) 
				mf *= mod_last * fb_mod_;

			if (noise_level_ > 0.0f)
				mf *= GeneratePinkNoise(&pink, rng) * noise_level_;
		
			mod_phase += mf;
		}
		else {
			carrier_phase += freq * dt;
			mod_phase += freq * mod_ratio * dt;
		}

		mod_last = mod_s;
		
		if (base_phase > 1.0f)
			base_phase -= 1.0f;

		if (carrier_phase > 1.0f)
			carrier_phase -= 1.0f;

		if (carrier_phase < -1.0f)
			carrier_phase += 1.0f;

		if (mod_phase > 1.0f)
			mod_phase -= 1.0f;

		if (mod_phase < -1.0f)
			mod_phase += 1.0f;




		filter.process(carrier_s);

		return lerp<float>(filter.lowpass() * envelope, filter.highpass() * envelope, filter_type);
		//return lerp<float>(filter.lowpass(), filter.highpass(), filter_type);
	}


	float wav2(const float& ratio_, const float& mod_level_, const float& noise_level_, const float& fb_carrier_, const float& fb_mod_) {
	
		const auto mod_s = interpvec<float>(tableB, abs(mod_phase) * 4159.0f);
		const auto carrier_s = interpvec<float>(tableC, abs(carrier_phase) * 4159.0f);
		const auto envelope = interpvec<float>(tableA, base_phase * 4159.0f);
		const auto mod_ratio = interparray<float, 12>(mod_ratios, ratio_ * 11.0f);

		freq += (target_freq - freq) * slew;
		base_freq += (base_target_freq - base_freq) * slew;
		
		base_phase += base_freq * dt;

		carrier_phase += freq * dt;
		if (carrier_phase > 1.0f)
			carrier_phase -= 1.0f;

		if (carrier_phase < -1.0f)
			carrier_phase += 1.0f; //2 is kinda fun


		carrier_phase += freq * mod_s * dt * mod_level_;
		if (carrier_phase > 1.0f)
			carrier_phase -= 1.0f;

		if (carrier_phase < -1.0f)
			carrier_phase += 1.0f; //2 is kinda fun

		mod_phase += freq * mod_ratio * dt;
		if (mod_phase > 1.0f)
			mod_phase -= 1.0f;

		if (mod_phase < -1.0f)
			mod_phase += 1.0f;  //ditto

		mod_phase += freq * mod_ratio * carrier_s * fb_carrier_ * dt;
		if (mod_phase > 1.0f)
			mod_phase -= 1.0f;

		if (mod_phase < -1.0f)
			mod_phase += 1.0f; //ditto
		
		mod_phase += freq * mod_ratio * mod_s * fb_mod_ * dt;
		if (mod_phase > 1.0f)
			mod_phase -= 1.0f;

		if (mod_phase < -1.0f)
			mod_phase += 1.0f; //ditto
	
		//hmmm

		mod_last = mod_s;
		
		if (base_phase > 1.0f)
			base_phase -= 1.0f;

		filter.process(lerp(carrier_s, GeneratePinkNoise(&pink, rng), noise_level_));
	
		return lerp<float>(filter.lowpass(), filter.highpass(), filter_type) * envelope;
	}


};




struct FmWTVoice : EngineUnit {
private:
	array<FMWTableOsc, POLY> oscillators; //Probably better to redo as a single unit

	int scopeIdx = 0;
	float scopeScale = 1.0f;

	//Going for 10 voices, 2 oscillators per note for detune max chord size of 5

public:

	FmWTVoice(const float sampleRate_, shared_ptr<ModMatrix> matrix_, const int channel_) {
		sampleRate = sampleRate_;
		matrix = matrix_;
		channel = channel_;

		shittyscope.resize(1024, 0.0f);

		for (auto& o : oscillators) {
			o.init(sampleRate);
		}

		//setVoiceFrequencies();
		interface.options = {
			{ "Ratio", 0.0f, 1.0f, nullptr, 1 },
			{ "Slew", 0.0f, 1.0f,  nullptr, 1 }
		};

		interface.consumers = {
			{ "Mod Level", 0.0f, 1.0f,     nullptr, POLY },
			{ "Carrier FB", 0.0f, 1.0f,    nullptr, POLY },
			{ "Mod FB", 0.0f, 1.0f,        nullptr, POLY },
			{ "Noise Level", 0.0f, 1.0f,   nullptr, POLY },
			{ "Resonance", 0.0f, 1.0f,     nullptr, POLY },
			{ "Filter",   0.0f, 1.0f,      nullptr, POLY },
			{ "Filter Type",   0.0f, 1.0f, nullptr, POLY },
			{ "Pan", 0.0f, 1.0f,           nullptr, POLY },
			{ "Gain", 0.0f, 1.0f,          nullptr, POLY },
		};

		registerInterface();

		//Set defaults... gotta make this a bit nicer to read
		*interface.options[0].resP = 0.0f;
		*interface.options[1].resP = 1.0f;

		for (int i = 0; i < POLY; i++) {
			interface.consumers[0].resP[i] = 0.0f; 
			interface.consumers[1].resP[i] = 0.0f; 
			interface.consumers[2].resP[i] = 0.0f; 
			interface.consumers[3].resP[i] = 0.0f; 
			interface.consumers[4].resP[i] = 0.0f; 
			interface.consumers[5].resP[i] = 0.5f; 
			interface.consumers[6].resP[i] = 0.0f; 
			interface.consumers[7].resP[i] = 0.5f; 
			interface.consumers[8].resP[i] = 0.0f; 
		}

	}

	pair<float, float> process_audio(Kit &kit_, array<float, POLY> & gates_) {
		const auto mod_ratio = *interface.options[0].resP;
		const auto slew = *interface.options[1].resP;

		const auto level_mod       = interface.consumers[0].resP;
		const auto level_fb_c	   = interface.consumers[1].resP;
		const auto level_fb_m	   = interface.consumers[2].resP;
		const auto level_noise	   = interface.consumers[3].resP;
		const auto resonance	   = interface.consumers[4].resP;
		const auto filter_cutoff   = interface.consumers[5].resP;
		const auto filter_response = interface.consumers[6].resP;
		const auto pan			   = interface.consumers[7].resP;
		const auto gain			   = interface.consumers[8].resP;

		pair<float, float> sample = make_pair(0.0f, 0.0f);

		for (int i = 0; i < POLY; i++) {
			oscillators[i].set_freq(kit_.tuning.freq(kit_.notes[i]), resonance[i], slew);
			oscillators[i].set_filter_cutoff(filter_cutoff[i]);
			oscillators[i].set_filter_type(filter_response[i]);

			const float vca = rescale<float>(powf(50.0f, gain[i]), 1.0f, 50.0f, 0.0f, 1.0f);

			if (vca > 0.0f) {
				const float monoSample = oscillators[i].wav2(mod_ratio, level_mod[i], level_noise[i], level_fb_c[i], level_fb_m[i]) * vca;

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

	virtual json toConfig() {
		json config = ModuleBase::toConfig();

		return config;
	}

	void loadConfig(const json cfg) {
		//Because of the order of things loading up again
		//The audio engine will load after the mod matrix wipe
		//but before the load, and get 2 interfaces
		//ModuleBase::loadConfig(cfg);

		for (int i = 0; i < interface.consumers.size(); i++) {
			interface.consumers[i].id = cfg["interface_consumer_ids"][i].get<int>();
		}

		for (int i = 0; i < interface.providers.size(); i++) {
			interface.providers[i].id = cfg["interface_provider_ids"][i].get<int>();
		}

		for (int i = 0; i < interface.options.size(); i++) {
			interface.options[i].id = cfg["interface_option_ids"][i].get<int>();
		}


	}

};


