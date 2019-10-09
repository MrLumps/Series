#pragma once
#include <cmath>

#include "Common.h"

//Adapted from https://www.youtube.com/watch?v=esjHXGPyrhg by Ivan Cohen
//Pretty sure this is Chamberlin form
//for stability o should not be > 2 which means @44100 sample rate
//this filter will top out at around 15540hz
struct StateVariableFilter {
private:
	float sampleRate = 1.0f;
	float state1 = 0.0f;
	float state2 = 0.0f;
	float lp = 0.0f;
	float bp = 0.0f;
	float hp = 0.0f;
	float o = 0.0f;
	float h = 0.0f;

public:
	float Q = 1.0f;

	void zero(const float sampleRate_) {
		sampleRate = sampleRate_;
		state1 = 0.0f;
		state2 = 0.0f;
		lp = 0.0f;
		bp = 0.0f;
		hp = 0.0f;
		o = 0.0f;
		h = 0.0f;
	}

	inline void setFreq(const float hz_) {
		o = tanf((hz_ / sampleRate) * PI);
		h = 1.0f / (1.0f + o / Q + o * o);
	}

	void process(const float in_) {
		hp = h * (in_ - (1.0f / Q + o) * state1 - state2);
		bp = o * hp + state1;
		state1 = o * hp + bp;
		lp = o * bp + state2;
		state2 = o * bp + lp;
	}

	float lowpass() {
		return lp;
	}

	float bandpass() {
		return bp;
	}

	float highpass() {
		return hp;
	}

	float notch() {
		return hp + lp;
	}

};

//One pole low pass
struct LowPass {
	float a = 0.928096f;
	float b = 0.481353f;
	float last = 0.0f;

public:

	float step(const float in_) {
		const float v = (b * in_) - (a * last);
		last = v;
		return v;
	}

	void setFreq(const float freq_, const float dt_) {
		const float omega = tanf((freq_ *dt_) * PI);
		a = (1.0f - omega) / (1.0f + omega);
		b = (1.0f - a) * 0.5f;
	}

};


//Simple 1 sample delay all pass
struct AllPassFilter {
private:
	float a1;
	float last;

public:
	AllPassFilter() {
		a1 = 1.0f;
		last = 0.0f;
	}

	//0-1
	void setResponse(const float r_) {
		a1 = (1.0f - r_) / (1.0f + r_);
	}

	float process(const float in_) {
		const float out = in_ * -a1 + last;
		last = out * a1 + in_;
		return out;
	}

};


//
//
//Total bleh, replace
//
//
struct StereoStateVariableFilter2 {
private:
	float panL(const float pan_) {
		return cosf(PI * (pan_) / 2.0f);
	}

	// 0 - 1
	float panR(const float pan_) {
		return sinf(PI * (pan_) / 2.0f);
	}

	float sampleRate = 1.0f;
	//state1, state2, lp, bp, hp
	array<float, 5> leftState;
	array<float, 5> rightState;

	float o_l = 0.0f;
	float h_l = 0.0f;
	float o_r = 0.0f;
	float h_r = 0.0f;

public:
	float Q = 1.0f;

	void zero(const float sampleRate_) {
		sampleRate = sampleRate_;
		fill(leftState.begin(), leftState.end(), 0.0f);
		fill(rightState.begin(), rightState.end(), 0.0f);
		o_l = 0.0f;
		h_l = 0.0f;
		o_r = 0.0f;
		h_r = 0.0f;
	}

	inline void setFreq(const float hz_, const float balance_) {
		o_l = tanf(( (hz_ * panL(balance_))/ sampleRate) * PI);
		h_l = 1.0f / (1.0f + o_l / Q + o_l * o_l);

		o_r = tanf(((hz_ * panR(balance_)) / sampleRate) * PI);
		h_r = 1.0f / (1.0f + o_r / Q + o_r * o_r);
	}

	void process(const pair<float, float> in_) {
		const float q_l = (1.0f / Q + o_l);

		leftState[4] = h_l * (in_.first - q_l * leftState[0] - leftState[1]);
		leftState[3] = o_l * leftState[4] + leftState[0];
		leftState[0] = o_l * leftState[4] + leftState[3];
		leftState[2] = o_l * leftState[3] + leftState[1];
		leftState[1] = o_l * leftState[3] + leftState[2];

		const float q_r = (1.0f / Q + o_r);
		rightState[4] = h_r * (in_.second - q_r * rightState[0] - rightState[1]);
		rightState[3] = o_r * rightState[4] + rightState[0];
		rightState[0] = o_r * rightState[4] + rightState[3];
		rightState[2] = o_r * rightState[3] + rightState[1];
		rightState[1] = o_r * rightState[3] + rightState[2];


	}

	pair<float, float> lowpass() {
		return make_pair(leftState[2], rightState[2]);
	}

	pair<float, float> bandpass() {
		return make_pair(leftState[3], rightState[3]);
	}

	pair<float, float> highpass() {
		return make_pair(leftState[4], rightState[4]);
	}

	pair<float, float> notch() {
		return make_pair(leftState[2] + leftState[4], rightState[2] + rightState[4]);
	}

};



struct StereoStateVariableFilter {
private:


	float sampleRate = 1.0f;
	//state1, state2, lp, bp, hp
	array<float, 5> leftState;
	array<float, 5> rightState;

	float o = 0.0f;
	float h = 0.0f;

public:
	float Q = 1.0f;

	void zero(const float sampleRate_) {
		sampleRate = sampleRate_;
		fill(leftState.begin(), leftState.end(), 0.0f);
		fill(rightState.begin(), rightState.end(), 0.0f);
		o = 0.0f;
		h = 0.0f;
	}

	inline void setFreq(const float hz_) {
		o = tanf((hz_ / sampleRate) * PI);
		h = 1.0f / (1.0f + o / Q + o * o);
	}

	void process(const pair<float, float> in_) {
		const float q = (1.0f / Q + o);

		leftState[4] = h * (in_.first - q * leftState[0] - leftState[1]);
		leftState[3] = o * leftState[4] + leftState[0];
		leftState[0] = o * leftState[4] + leftState[3];
		leftState[2] = o * leftState[3] + leftState[1];
		leftState[1] = o * leftState[3] + leftState[2];

		rightState[4] = h * (in_.second - q * rightState[0] - rightState[1]);
		rightState[3] = o * rightState[4] + rightState[0];
		rightState[0] = o * rightState[4] + rightState[3];
		rightState[2] = o * rightState[3] + rightState[1];
		rightState[1] = o * rightState[3] + rightState[2];


	}

	pair<float, float> lowpass() {
		return make_pair(leftState[2], rightState[2]);
	}

	pair<float, float> bandpass() {
		return make_pair(leftState[3], rightState[3]);
	}

	pair<float, float> highpass() {
		return make_pair(leftState[4], rightState[4]);
	}

	pair<float, float> notch() {
		return make_pair(leftState[2] + leftState[4], rightState[2] + rightState[4]);
	}

};



struct DCBlocker {
	float x1 = 0.0f;
	float y1 = 0.0f;

	float process(const float in_) {
		const float res = in_ - x1 + 0.995f * y1;
		x1 = in_;
		y1 = res;
		return res;
	}

	void reset() {
		x1 = 0.0f;
		y1 = 0.0f;
	}

};

//Simple one pole hp / hp
struct RCFilter {
	float sampleRate = 1.0f;
	float o = 0.0f;
	float state1 = 0.0f;
	float state2 = 0.0f;

	void zero(const float sampleRate_) {
		sampleRate = sampleRate_;
		o = 0.0f;
		state1 = 0.0f;
		state2 = 0.0f;
	}

	inline void setFreq(const float hz_) {
		o = 2.0f / (hz_ / sampleRate);
	}

	void process(const float in_) {
		const float state = (in_ + state1 - state2 * (1.0f - o)) / (1.0f + o);
		state1 = in_;
		state2 = state;
	}

	float lowpass() {
		return state2;
	}

	float highpass() {
		return state1 - state2;
	}


};


struct StateVariableFilter2 {
private:
	float sample_rate = 0.0f;
	float state1 = 0.0f;
	float state2 = 0.0f;
	float lp = 0.0f;
	float bp = 0.0f;
	float hp = 0.0f;
	float o = 0.0f;
	float h = 0.0f;
	float r = 0.0f;

public:

	void zero(const float sample_rate_) {
		sample_rate = sample_rate_;
		state1 = 0.0f;
		state2 = 0.0f;
		lp = 0.0f;
		bp = 0.0f;
		hp = 0.0f;
		o = 0.0f;
		h = 0.0f;
		r = 0.0f;
	}

	//hz, 1-6ish, 0-1
	void setFreq(const float hz_, const float r_) {
		r = 1.0f / r_;
		o = tanf((hz_ / sample_rate) * PI);
		h = 1.0f / (1.0f + r * o + o * o);
	}

	void process(const float in_) {
		hp = h * (in_ - r * state1 - o * state1 - state2);
		bp = o * hp + state1;
		state1 = o * hp + bp;
		lp = o * bp + state2;
		state2 = o * bp + lp;
	}

	float lowpass() {
		return lp;
	}

	float bandpass() {
		return bp;
	}

	float highpass() {
		return hp;
	}

};













