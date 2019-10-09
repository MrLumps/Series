/*
Improved Moog has been adapted from
https://github.com/ddiakopoulos/MoogLadders/blob/master/src/ImprovedModel.h

Copyright 2012 Stefano D'Angelo <zanga.mail@gmail.com>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#pragma once

#include <vector>

#define VT 0.312
#define M_PI       3.14159265358979323846

struct ImprovedMoog {
	std::vector<double> V;
	std::vector<double> dV;
	std::vector<double> tV;
	
	double x;
	double g;
	double drive;

	double sampleRate;
	double cutoff;
	double resonance;

	double hp;
	double lp;

	double rate;
	const double thermal = 2.0 * 0.312;

public:

	ImprovedMoog(const float sampleRate_) {
		V.resize(4, 0.0);
		dV.resize(4, 0.0);
		tV.resize(4, 0.0);

		sampleRate = sampleRate_;
		rate = 2.0 * sampleRate;
		drive = 0.0;
		cutoff = 1000.0f;
		resonance = 0.1f;
		x = 0.0;
		g = 0.0;
		hp = 0.0;
		lp = 0.0;
	}

	ImprovedMoog() {
		V.resize(4, 0.0);
		dV.resize(4, 0.0);
		tV.resize(4, 0.0);
		sampleRate = 1.0f;
		rate = 2.0f;
		drive = 0.0;
		cutoff = 1000.0f;
		resonance = 0.1f;
		x = 0.0;
		g = 0.0;
		hp = 0.0;
		lp = 0.0;
	}

	void setSampleRate(const float sampleRate_) {
		sampleRate = sampleRate_;
		rate = 2.0 * sampleRate;
	}

	void process(float in_) {
		double dV0, dV1, dV2, dV3;

		const float signal = drive * in_;

		dV0 = -g * (tanh((signal + resonance * V[3]) / thermal) + tV[0]);
		V[0] += (dV0 + dV[0]) / rate;
		dV[0] = dV0;
		tV[0] = tanh(V[0] / thermal);

		dV1 = g * (tV[0] - tV[1]);
		V[1] += (dV1 + dV[1]) / rate;
		dV[1] = dV1;
		tV[1] = tanh(V[1] / thermal);

		dV2 = g * (tV[1] - tV[2]);
		V[2] += (dV2 + dV[2]) / rate;
		dV[2] = dV2;
		tV[2] = tanh(V[2] / thermal);

		dV3 = g * (tV[2] - tV[3]);
		V[3] += (dV3 + dV[3]) / rate;
		dV[3] = dV3;
		tV[3] = tanh(V[3] / thermal);

		lp = V[3];
		hp = signal - lp;

	}

	// [0, 4]
	void SetResonance(const double r_) {
		resonance = r_;
	}

	// [0, 2]
	void SetDrive(const double d_) {
		drive = d_;
	}

	const float HighPass() {
		return (float)hp;
	}

	const float LowPass() {
		return (float)lp;
	}

	// normalized cutoff frequency
	void setFreq(const double hz_) {
		cutoff = hz_;
		x = (M_PI * cutoff) / sampleRate;
		g = 4.0 * M_PI * VT * cutoff * (1.0 - x) / (1.0 + x);
	}

};