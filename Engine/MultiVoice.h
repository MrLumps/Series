#pragma once
#include <math.h>
#include <vector>
#include <string>
#include <memory>

#pragma comment(lib, "stmlib.lib")

#include "BaseObjects.h"
#include "Trigger.h"

//no arm asm thanks
//#define TEST
#include "../Mutable/plaits/dsp/voice.h"

struct MultiVoice : EngineUnit {
private:
	inline float clamp(const float x_, const float min_, const float max_) {
		return fmaxf(fminf(x_, fmaxf(min_, max_)), fminf(min_, max_));
	}

	plaits::Voice voice;
	plaits::Patch patch;
	plaits::Modulations modulations;
	array<char, 16384> buffer;
	array<Trigger, POLY> triggers;

	int current_gate = 0;

	int scopeIdx = 0;
	float scopeScale = 1.0f;


public:

	

	MultiVoice(const float sampleRate_, shared_ptr<ModMatrix> matrix_, const int channel_) {
		sampleRate = sampleRate_;
		matrix = matrix_;
		channel = channel_;

		buffer.fill(0);

		stmlib::BufferAllocator allocator(buffer.data(), sizeof(buffer));
		voice.Init(&allocator);

		memset(&patch, 0, sizeof(patch));
		memset(&modulations, 0, sizeof(modulations));

		patch.engine = 0;

		modulations.frequency_patched = true;
		modulations.timbre_patched = true;
		modulations.morph_patched = true;
		modulations.trigger_patched = true;
		modulations.level_patched = true;

		scopeIdx = 0;
		shittyscope.resize(1024, 0.0f);

		interface.options = {
			{ "Fm Amount",     -4.0f, 4.0f, nullptr, 1},
			{ "Timbre Amount", -1.0f, 1.0f, nullptr, 1},
			{ "Morph Amount",  -1.0f, 1.0f, nullptr, 1}
		};

		interface.consumers = {
			{ "Low Pass Gate", 0.0f, 1.0f, nullptr, POLY},
			{ "Harmonics",     0.0f, 1.0f, nullptr, POLY},
			{ "Timbre",        0.0f, 1.0f, nullptr, POLY},
			{ "Morph",         0.0f, 1.0f, nullptr, POLY},
			{ "Engine",        0.0f, 1.0f, nullptr, POLY},
			{ "Fm",            0.0f, 1.0f, nullptr, POLY},
			{ "Mix",           0.0f, 1.0f, nullptr, POLY},
			{ "Decay",         0.0f, 1.0f, nullptr, POLY},
			{ "Pan",           0.0f, 1.0f, nullptr, POLY},
			{ "Gain",          0.0f, 1.0f, nullptr, POLY}
		};

		registerInterface();

		for (int i = 0; i < POLY; i++) {
			interface.consumers[0].resP[i] = 0.0f;
			interface.consumers[1].resP[i] = 0.0f;
			interface.consumers[2].resP[i] = 0.0f;
			interface.consumers[3].resP[i] = 0.0f;
			interface.consumers[4].resP[i] = 0.0f;
			interface.consumers[5].resP[i] = 0.0f;
			interface.consumers[6].resP[i] = 0.0f;
			interface.consumers[7].resP[i] = 0.0f;
			interface.consumers[8].resP[i] = 0.5f; // set pan
			interface.consumers[9].resP[i] = 0.0f;
		}

	}

	void next_voice() {
		patch.engine++;
		if (patch.engine > 16)
			patch.engine = 0;
	}

	void prev_voice() {
		patch.engine--;
		if (patch.engine < 0)
			patch.engine = 15;
	}

	const int active_engine() {
		return voice.active_engine();
	}

	//Okay we'll just setup triggers for each gate
	//
	pair<float, float> process_audio(Kit &kit_, array<float, POLY> & gates_) {
		const auto low_pass_gate_in = interface.consumers[0].resP;
		const auto harmonics_in		= interface.consumers[1].resP;
		const auto timbre_in		= interface.consumers[2].resP;
		const auto morph_in			= interface.consumers[3].resP;
		const auto engine_in		= interface.consumers[4].resP;
		const auto fm_in			= interface.consumers[5].resP;
		const auto mix_in			= interface.consumers[6].resP;
		const auto decay_in			= interface.consumers[7].resP;
		const auto pan_in			= interface.consumers[8].resP;
		const auto gain_in			= interface.consumers[9].resP;

		const auto fm_amount        = *interface.options[0].resP;
		const auto timbre_amount    = *interface.options[1].resP;
		const auto morph_amount     = *interface.options[2].resP;

		
		
		modulations.trigger = 0.0f;
		for (int i = 0; i < POLY; i++) {
			if (triggers[i].process(gates_[i])) {
				current_gate = i;
				patch.note = static_cast<float>(kit_.notes[current_gate].n);
				//modulations.note = static_cast<float>(kit_.notes[current_gate].n); //hmmm
				modulations.trigger = 1.0f;
			}
		}

		patch.harmonics = harmonics_in[current_gate];
		patch.timbre = timbre_in[current_gate];
		patch.morph = timbre_in[current_gate];
		patch.lpg_colour = low_pass_gate_in[current_gate];
		
		//Hmmm de-poly this perhaps
		patch.decay = morph_in[current_gate];

		patch.frequency_modulation_amount = fm_amount;
		patch.timbre_modulation_amount = timbre_amount;
		patch.morph_modulation_amount = morph_amount;

		modulations.engine = engine_in[current_gate];
		
		modulations.frequency = fm_in[current_gate];
		modulations.harmonics = harmonics_in[current_gate];
		modulations.timbre = timbre_in[current_gate];
		modulations.morph = morph_in[current_gate];
		// Triggers at around 0.7 V
		
		modulations.level = gain_in[current_gate];

		plaits::Voice::Frame output[1];
		voice.Render(patch, modulations, output, 1);
		const auto mixed = lerp<float>(output->out * (1.0f/32768.0f), output->aux * (1.0f/32768.0f), mix_in[current_gate]);

		pair<float, float> sample = make_pair(mixed * panL(pan_in[current_gate]), mixed * panR(pan_in[current_gate]));

		shittyscope[scopeIdx] = get<0>(sample);
		scopeIdx++;
		if (scopeIdx >= shittyscope.size()) {
			scopeIdx = 0;
		}

		return sample;

	}

	//voices need this because they get id's after they are instanitated
	//so any ids we got from being created need to go 
	//maybe need another function


};

