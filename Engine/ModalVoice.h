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
//Set in project
#include "../Mutable/elements/dsp/part.h"
#include "../libsamplerate/samplerate.h"

//
//not good
//using namespace std;
//struct SampleRateConverter {
//	SRC_STATE* state = nullptr;
//	SRC_DATA data;
//
//	//converter_type:
//	//SRC_SINC_BEST_QUALITY = 0,
//	//SRC_SINC_MEDIUM_QUALITY = 1,
//	//SRC_SINC_FASTEST = 2,
//	//SRC_ZERO_ORDER_HOLD = 3,
//	//SRC_LINEAR = 4,
//	//ratio input sample rate / output sample rate
//
//	void init(const int converter_type, const float ratio) {
//		int ret;
//		state = src_new(converter_type, 1, &ret);
//		assert(!ret);
//
//		src_set_ratio(state, ratio);
//		data.src_ratio = ratio;
//		data.end_of_input = false;
//	}
//
//	~SampleRateConverter() {
//		src_delete(state);
//	}
//
//	void convert(const array<float, 16> input, int &input_used, array<float, 24> &output, int& output_used) {
//
//		data.data_in = input.data();
//		data.input_frames = (long)input.size();
//		data.data_out = output.data();
//		data.output_frames = (long)output.size();
//		src_process(state, &data);
//		input_used = data.input_frames_used;
//		output_used = data.output_frames_gen;
//	}
//
//	void reset() {
//		src_reset(state);
//	}
//
//};

struct ModalVoice : EngineUnit {
private:
	inline float clamp(const float x_, const float min_, const float max_) {
		return fmaxf(fminf(x_, fmaxf(min_, max_)), fminf(min_, max_));
	}

	array<uint16_t, 32768> reverb_buffer;

	//SampleRateConverter sample_rate_input;
	//SampleRateConverter sample_rate_output;

	array<float, 16> part_blow_buffer;
	array<float, 16> part_strike_buffer;

	array<float, 16> part_main_buffer;
	array<float, 16> part_aux_buffer;

	array<float, 24> output_buffer_l;
	array<float, 24> output_buffer_r;
	
	int buffer_idx = 0;
	
	elements::Part* part;

	int output_length = 0;
	int scopeIdx = 0;
	float scopeScale = 1.0f;


public:

	

	ModalVoice(const float sampleRate_, shared_ptr<ModMatrix> matrix_, const int channel_) {
		sampleRate = sampleRate_;
		matrix = matrix_;
		channel = channel_;
		
		//sample_rate_input.init(SRC_SINC_FASTEST, 32000.0f / sampleRate);
		//sample_rate_output.init(SRC_SINC_MEDIUM_QUALITY, sampleRate / 32000.0f);

		part_blow_buffer.fill(0.0f);
		part_strike_buffer.fill(0.0f);
		part_main_buffer.fill(0.0f);
		part_aux_buffer.fill(0.0f);

		output_buffer_l.fill(0.0f);
		output_buffer_r.fill(0.0f);

		reverb_buffer.fill(0);

		buffer_idx = 0;
		output_length = 0;

		part = new elements::Part();
		part->Init(reverb_buffer.data());
		uint32_t seed[3] = { 7, 3, 8 };
		part->Seed(seed, 3);
		part->set_easter_egg(true);
		//part->set_resonator_model(static_cast<elements::ResonatorModel>(2)); //0 normal, 1 string, 2 chords

		shittyscope.resize(1024, 0.0f);

		interface.options = {
			{ "Fm", 0.0f, 1.0f,            nullptr, 1},
			{ "Contour", 0.0f, 1.0f,       nullptr, 1},
			{ "Bow", 0.0f, 1.0f,           nullptr, 1},
			{ "Bow level", 0.0f, 1.0f,     nullptr, 1},
			{ "Bow Timbre", 0.0f, 1.0f,    nullptr, 1},
			{ "Blow Level", 0.0f, 1.0f,    nullptr, 1},
			{ "Blow Timbre", 0.0f, 1.0f,   nullptr, 1},
			{ "Flow", 0.0f, 1.0f,          nullptr, 1},
			{ "Mallet", 0.0f, 1.0f,        nullptr, 1},
			{ "Mallet Level", 0.0f, 1.0f,  nullptr, 1},
			{ "Strike Timbre", 0.0f, 1.0f, nullptr, 1},
			{ "Geometry", 0.0f, 1.0f,      nullptr, 1 },
			{ "Brightness", 0.0f, 1.0f,    nullptr, 1 },
			{ "Position", 0.0f, 1.0f,      nullptr, 1 },
			{ "Damping", 0.0f, 1.0f,       nullptr, 1 },
			{ "Strength", 0.0f, 1.0f,      nullptr, 1 },
			{ "Space", 0.0f, 1.0f,         nullptr, 1 },
			{ "Gain", 0.0f, 1.0f,          nullptr, 1 },
		};


		interface.consumers = {
			{ "Blow", 0.0f, 1.0f,   nullptr, POLY},
			{ "Strike", 0.0f, 1.0f, nullptr, POLY},
		};

		registerInterface();

		*interface.options[0].resP = 0.0f;
		*interface.options[1].resP = 0.5f;
		*interface.options[2].resP = 0.5f;
		*interface.options[3].resP = 0.5f;
		*interface.options[4].resP = 0.5f;
		*interface.options[5].resP = 0.5f;
		*interface.options[6].resP = 0.5f;
		*interface.options[7].resP = 0.5f;
		*interface.options[8].resP = 0.5f;
		*interface.options[9].resP = 0.5f;
		*interface.options[10].resP = 0.5f;
		*interface.options[11].resP = 0.5f;
		*interface.options[12].resP = 0.0f;
		*interface.options[13].resP = 0.5f;
		*interface.options[14].resP = 0.1f;
		*interface.options[15].resP = 0.1f;
		*interface.options[16].resP = 0.5f;
		*interface.options[17].resP = 0.0f;

	}

	~ModalVoice() {
		delete part;
	}

	
	pair<float, float> process_audio(Kit &kit_, array<float, POLY> & gates_) {

		const auto blow_in  = interface.consumers[0].resP;
		const auto strike_in = interface.consumers[1].resP;

		float fm = *interface.options[0].resP;
		float contour = *interface.options[1].resP;
		float bow = *interface.options[2].resP;
		float bow_level = *interface.options[3].resP;
		float bow_timbre = *interface.options[4].resP;
		float blow_level = *interface.options[5].resP;
		float blow_timbre = *interface.options[6].resP;
		float flow = *interface.options[7].resP;
		float mallet = *interface.options[8].resP;
		float mallet_level = *interface.options[9].resP;
		float strike_timbre = *interface.options[10].resP;
		float geometry = *interface.options[11].resP;
		float brightness = *interface.options[12].resP;
		float position = *interface.options[13].resP;
		float damping = *interface.options[14].resP;
		float strength = *interface.options[15].resP;
		float space = *interface.options[16].resP;
		float gain = *interface.options[17].resP;



		int current_gate = 0;
		
		for (int i = POLY-1; i >= 0; i--) {
			if (gates_[i] > 0.000001f) {
				current_gate = i;
				continue;
			}
		}
		
		elements::Patch* patch = part->mutable_patch();
		patch->exciter_envelope_shape = contour;
		patch->exciter_bow_level = bow;
		patch->exciter_blow_level = blow_level;
		patch->exciter_strike_level = mallet_level;
		patch->exciter_bow_timbre = bow_timbre;
		patch->exciter_blow_meta = flow;
		patch->exciter_blow_timbre = blow_timbre;
		patch->exciter_strike_meta = mallet;
		patch->exciter_strike_timbre = strike_timbre;
		patch->resonator_geometry = geometry;
		patch->resonator_damping = damping;
		patch->resonator_position = position;
		patch->resonator_brightness = brightness;
		patch->space = space;
		
		
		
		elements::PerformanceState performance;
		performance.note = static_cast<float>(kit_.notes[current_gate].n); //yes it is
		//performance.note = 0.0f;
		performance.modulation = fm;
		performance.gate = gates_[current_gate];
		performance.strength = strength;
		
		float main = 0.0f;
		float aux = 0.0f;
		
		part->Process(performance, &blow_in[current_gate], &strike_in[current_gate], &main, &aux, 1);
		


		shittyscope[scopeIdx] = main;
		scopeIdx++;
		if (scopeIdx >= shittyscope.size()) {
			scopeIdx = 0;
		}

		return make_pair(main,aux);

	}

};

