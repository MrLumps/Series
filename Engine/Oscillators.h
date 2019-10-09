#pragma once
#include <vector>
#include <array>
#include <math.h>

#include "Common.h"
#include "Delay.h"
#include "Filter.h"

using namespace std;

extern const vector<float> tableA;
extern const vector<float> tableB;
extern const vector<float> tableC;
extern const vector<float> tableD;
extern const vector<float> table_hsin;


struct basicOsc {
private:
	float phase = 0.0f;

public:
	float freq = 0.0f;
	float dt = 1.0f / 44100.0f;


	void zero(const float sampleRate_) {
		dt = 1.0f / sampleRate_;
		phase = 0.0f;
		freq = 0.0f;
	}

	//0-1
	float sineDC() {
		phase += freq * dt;

		if (phase >= 1.0) {
			phase -= 1.0;
		}

		return (aproxSine(2.0f * PI * phase) + 1.0f) * 0.5f;
	}

	float triDC() {

		float tri;
		if (phase < 0.25f)
			tri = 4.0f * phase;
		else if (phase < 0.75f)
			tri = 2.0f - 4.0f * phase;
		else
			tri = -4.0f + 4.0f * phase;

		phase += freq * dt;

		if (phase >= 1.0f) {
			phase -= 1.0f;
		}
		return tri + 0.5f * 0.5f;
	}

	//-1,1
	float sineAC() {
		phase += freq * dt;

		if (phase >= 1.0) {
			phase -= 1.0;
		}

		return aproxSine(2.0f * PI * phase);
	}

};



//struct slightlyFancyDCOsc {
//private:
//	float phase = 0.0f;
//
//public:
//	float freq = 0.0f;
//	float level = 0.0f;
//	float dt = 1.0f / 44100.0f;
//
//
//	void zero(const float sampleRate_) {
//		dt = 1.0f / sampleRate_;
//		phase = 0.0f;
//		freq = 0.0f;
//		level = 0.0f;
//	}
//
//
//	void step(float dt) {
//		float deltaPhase = fminf(freq * dt, 0.5);
//		phase += deltaPhase;
//
//		if (phase >= 1.0) {
//			phase -= 1.0;
//		}
//
//	}
//
//
//	float sine() {
//		return ((sinf(2.0f * PI * phase) + 1.0f) / 2.0f) * level;
//	}
//
//	float tri() {
//		float tri;
//		if (phase < 0.25f)
//			tri = 4.0f * phase;
//		else if (phase < 0.75)
//			tri = 2.0f - 4.0f * phase;
//		else
//			tri = -4.0f + 4.0f * phase;
//		return ((tri + 1.0f) / 2.0f) * level;
//	}
//
//	float saw() {
//		float saw;
//		if (phase < 0.5f)
//			saw = 2.0f * phase;
//		else
//			saw = -2.0f + 2.0f * phase;
//		return ((saw + 1.0f) / 2.0f) * level;
//	}
//
//	float sqr() {
//		float sqr = phase < 0.5f ? 1.0f : -1.0f;
//		return ((sqr + 1.0f) / 2.0f) * level;
//	}
//
//	float hsin() {
//		return ((sinf(PI * phase) + 1.0f) / 2.0f) * level;
//	}
//
//	float qsin() {
//		return ((sinf((PI / 2.0f) * phase) + 1.0f) / 2.0f) * level;
//	}
//
//
//
//};








////Note each tracks 2 voices P and S for detune
//struct simpleWTableOsc {
//private:
//	//const float slewMin = 0.1;
//	//const float slewMax = 10000.0;
//	const float shapeScale = 1 / 10.0;
//	const float minCutoff = 20.0f;
//	const float maxCutoff = 10560.0f;
//
//	StateVariableFilter filter;
//	float filterType = 0.0f;
//
//	float dIdxP = 0.0f; 
//	float dIdxS = 0.0f;
//	float idxP = 0.0f;
//	float idxS = 0.0f;
//	float sampleRate = 1.0f;
//	float freqMultiplier = 1.0f;
//	float freq = 0.0f;
//	
//public:
//	float mixX = 0.0f;
//	float mixY = 0.0f;
//
//	simpleWTableOsc(const float sampleRate_) {
//		sampleRate = sampleRate_;
//		filterType = 0.0f;
//		dIdxP = 0.0f;
//		dIdxS = 0.0f;
//		idxP = 0.0f;
//		idxS = 0.0f;
//		mixX = 0.0f;
//		mixY = 0.0f;
//		freqMultiplier = 1.0f;
//		freq = 0.0f;
//
//		filter.zero(sampleRate);
//	}
//
//	void setFreq(const float hz_) {
//		//cout << "Freq set to " << hz_ << " hz " << endl;
//		freq = hz_;
//	}
//
//	//0-1
//	void setFilterCutoff(const float c_) {
//		filter.setFreq(minCutoff * powf(maxCutoff / minCutoff, c_));
//	}
//
//	void setFilterType(const float c_) {
//		filterType = c_;
//	}
//
//	//0-1
//	void setDetune(const float octave_) {
//		const auto cents = octave_ * 1200.0f;
//		freqMultiplier = roundf(100000.0f * powf(2.0f, (floorf(cents) / 100.0f / 12.0f))) / 100000.0f;
//	}
//
// 
//	void setMix(const float mX_, const float mY_) {
//		mixX = mX_;
//		mixY = mY_;
//	}
//
//
//	float wav() {
//		float intpartP, intpartS = 0;
//		float fracpartP = modf(idxP, &intpartP);
//		float fracpartS = modf(idxS, &intpartS);
//	
//		dIdxP = (freq) / (sampleRate / static_cast<float>(4160));
//		dIdxS = (freq * freqMultiplier) / (sampleRate / static_cast<float>(4160));
//
//		if (intpartP > 4160 - 1) {
//			intpartP = 0;
//		}
//
//		if (intpartS > 4160 - 1) {
//			intpartS = 0;
//		}
//
//		//apparently broken allpass interp.... try real allpass someday
//		const auto sampA = (1.0f - fracpartP) * tableA[static_cast<int>(intpartP)] + tableA[(static_cast<int>(intpartP) + 1) % 4160] * fracpartP;
//		const auto sampB = (1.0f - fracpartS) * tableA[static_cast<int>(intpartS)] + tableA[(static_cast<int>(intpartS) + 1) % 4160] * fracpartS;
//		const auto sampC = (1.0f - fracpartP) * tableB[static_cast<int>(intpartP)] + tableB[(static_cast<int>(intpartP) + 1) % 4160] * fracpartP;
//		const auto sampD = (1.0f - fracpartS) * tableB[static_cast<int>(intpartS)] + tableB[(static_cast<int>(intpartS) + 1) % 4160] * fracpartS;
//		const auto sampE = (1.0f - fracpartP) * tableC[static_cast<int>(intpartP)] + tableC[(static_cast<int>(intpartP) + 1) % 4160] * fracpartP;
//		const auto sampF = (1.0f - fracpartS) * tableC[static_cast<int>(intpartS)] + tableC[(static_cast<int>(intpartS) + 1) % 4160] * fracpartS;
//		const auto sampG = (1.0f - fracpartP) * tableD[static_cast<int>(intpartP)] + tableD[(static_cast<int>(intpartP) + 1) % 4160] * fracpartP;
//		const auto sampH = (1.0f - fracpartS) * tableD[static_cast<int>(intpartS)] + tableD[(static_cast<int>(intpartS) + 1) % 4160] * fracpartS;
//
//		const float outX1P = lerp<float>(sampA, sampC, mixX);
//		const float outX2P = lerp<float>(sampE, sampG, mixX);
//		const float outP = lerp<float>(outX1P, outX2P, mixY);
//
//		const float outX1S = lerp<float>(sampB, sampD, mixX);
//		const float outX2S = lerp<float>(sampF, sampH, mixX);
//		const float outS = lerp<float>(outX1S, outX2S, mixY);
//
//		idxP = fracpartP + intpartP + dIdxP;
//		idxS = fracpartS + intpartS + dIdxS;
//
//		const float preFilter = (outP + outS) / 2.0f;
//		filter.process(preFilter);
//		return lerp<float>(filter.lowpass(), filter.highpass(), filterType);
//
//	}
//
//
//};




//struct STF {
//private:
//	const float minCutoff = 20.0f;
//	const float maxCutoff = 10560.0f;
//	float sampleRate = 1.0;
//
//	array<float, POLY> state1;
//	array<float, POLY> state2;
//	array<float, POLY> lp;
//	array<float, POLY> bp;
//	array<float, POLY> hp;
//	array<float, POLY> o;
//	array<float, POLY> h;
//
//public:
//	float Q = 1.0f;
//
//	//0-1
//	array<float, POLY> cutOffParam;
//
//	void zero(const float sampleRate_) {
//		sampleRate = sampleRate_;
//		state1.fill(0.0f);
//		state2.fill(0.0f);
//		lp.fill(0.0f);
//		bp.fill(0.0f);
//		hp.fill(0.0f);
//		o.fill(0.0f);
//		h.fill(0.0f);
//		cutOffParam.fill(0.0f);
//	}
//
//
//	void process(array<float, POLY>& in_) {
//
//		#pragma omp simd
//		for (int i = 0; i < POLY; i++) {
//			o[i] = tanf(((minCutoff * powf(maxCutoff / minCutoff, cutOffParam[i])) / sampleRate) * PI);
//			h[i] = 1.0f / (1.0f + o[i] / Q + o[i] * o[i]);
//		}
//
//		#pragma omp simd
//		for (int i = 0; i < POLY; i++) {
//			hp[i] = h[i] * (in_[i] - (1.0f / Q + o[i]) * state1[i] - state2[i]);
//			bp[i] = o[i] * hp[i] + state1[i];
//			state1[i] = o[i] * hp[i] + bp[i];
//			lp[i] = o[i] * bp[i] + state2[i];
//			state2[i] = o[i] * bp[i] + lp[i];
//		}
//	}
//
//	array<float, POLY>* lowpass() {
//		return &lp;
//	}
//
//	array<float, POLY>* bandpass() {
//		return &bp;
//	}
//
//	array<float, POLY>* highpass() {
//		return &hp;
//	}
//
//	//Well that won't work
//	//array<float, 8>* notch() {
//	//	return hp + lp;
//	//}
//
//};



//struct BlockWTableOsc {
//private:
//
//	const float shapeScale = 1 / 10.0;
//	float tableRate = 1.0;
//	
//	array<float, POLY> dIdxP;
//	array<float, POLY> dIdxS;
//	array<float, POLY> idxP;
//	array<float, POLY> idxS;
//
//	array<float, POLY> intpartP;
//	array<float, POLY> intpartS;
//	array<float, POLY> fracpartP;
//	array<float, POLY> fracpartS;
//
//	
//
//	array<array<float, POLY>, POLY> buffer; //The outside 8 is poly the inside is the samples for the lerps and just happens to be 8
//	
//
//	array<float, POLY> outA;
//	array<float, POLY> outB;
//	array<float, POLY> outC;
//	array<float, POLY> outD;
//
//	array<float, POLY> outP;
//	array<float, POLY> outS;
//
//	//y(n) = x(n - (M + 1)
//	
//
//public:
//	array<float, POLY> filterType;
//	array<float, POLY> freqMultiplier;
//	array<float, POLY> freq;
//	array<float, POLY> mixX;
//	array<float, POLY> mixY;
//
//	STF filter;
//
//	BlockWTableOsc(const float sampleRate_) {
//		tableRate = sampleRate_ / static_cast<float>(4160);
//		fill(filterType.begin(), filterType.end(), 0.0f);
//		fill(dIdxP.begin(), dIdxP.end(), 0.0f);
//		fill(dIdxS.begin(), dIdxS.end(), 0.0f);
//		fill(idxP.begin(), idxP.end(), 0.0f);
//		fill(idxS.begin(), idxS.end(), 0.0f);
//		fill(intpartP.begin(), intpartP.end(), 0.0f);
//		fill(intpartS.begin(), intpartS.end(), 0.0f);
//		fill(fracpartP.begin(), fracpartP.end(), 0.0f);
//		fill(fracpartS.begin(), fracpartS.end(), 0.0f);
//		fill(freqMultiplier.begin(), freqMultiplier.end(), 0.0f);
//		fill(freq.begin(), freq.end(), 0.0f);
//		fill(mixX.begin(), mixX.end(), 0.0f);
//		fill(mixY.begin(), mixY.end(), 0.0f);
//		fill(outA.begin(), outA.end(), 0.0f);
//		fill(outB.begin(), outB.end(), 0.0f);
//		fill(outC.begin(), outC.end(), 0.0f);
//		fill(outD.begin(), outD.end(), 0.0f);
//		fill(outP.begin(), outP.end(), 0.0f);
//		fill(outS.begin(), outS.end(), 0.0f);
//
//		for (auto& b : buffer) {
//			for (auto& v : b) {
//				v = 0.0f;
//			}
//		}
//
//		filter.zero(sampleRate_);
//	}
//
//
//	//0-1
//	//void setDetune(const array<float, 8> octave_) {
//	//	for (int i = 0; i < 8; i++) {
//	//		freqMultiplier[i] = round(100000.0f * pow(2.0f, (floor(octave_[i] * 1200.0) / 100.0f / 12.0f))) / 100000.0f;
//	//	}
//	//}
//
//	void wav(array<float, POLY> &in_) {
//
//		for (int i = 0; i < POLY; i++) {
//			intpartP[i] = 0.0f;
//			intpartS[i] = 0.0f;
//			fracpartP[i] = modf(idxP[i], &intpartP[i]);
//			fracpartS[i] = modf(idxS[i], &intpartS[i]);
//
//			dIdxP[i] = (freq[i]) / tableRate;
//			dIdxS[i] = (freq[i] * freqMultiplier[i]) / tableRate;
//
//			if (intpartP[i] > 4160 - 1) {
//				intpartP[i] = 0;
//			}
//
//			if (intpartS[i] > 4160 - 1) {
//				intpartS[i] = 0;
//			}
//
//		}
//
//
//		//This is bad since it depends on the size of POLY
//		//If poly changes, see commented lines
//		//Fix
//		#pragma omp simd
//		for (int i = 0; i < POLY; i++) {
//			buffer[0][i] = (1.0f - fracpartP[i]) * tableA[static_cast<int>(intpartP[i])] + tableA[(static_cast<int>(intpartP[i]) + 1) % 4160] * fracpartP[i];
//			buffer[1][i] = (1.0f - fracpartS[i]) * tableA[static_cast<int>(intpartS[i])] + tableA[(static_cast<int>(intpartS[i]) + 1) % 4160] * fracpartS[i];
//			buffer[2][i] = (1.0f - fracpartP[i]) * tableB[static_cast<int>(intpartP[i])] + tableB[(static_cast<int>(intpartP[i]) + 1) % 4160] * fracpartP[i];
//			buffer[3][i] = (1.0f - fracpartS[i]) * tableB[static_cast<int>(intpartS[i])] + tableB[(static_cast<int>(intpartS[i]) + 1) % 4160] * fracpartS[i];
//			//buffer[4][i] = (1.0f - fracpartP[i]) * tableC[static_cast<int>(intpartP[i])] + tableC[(static_cast<int>(intpartP[i]) + 1) % 4160] * fracpartP[i];
//			//buffer[5][i] = (1.0f - fracpartS[i]) * tableC[static_cast<int>(intpartS[i])] + tableC[(static_cast<int>(intpartS[i]) + 1) % 4160] * fracpartS[i];
//			//buffer[6][i] = (1.0f - fracpartP[i]) * tableD[static_cast<int>(intpartP[i])] + tableD[(static_cast<int>(intpartP[i]) + 1) % 4160] * fracpartP[i];
//			//buffer[7][i] = (1.0f - fracpartS[i]) * tableD[static_cast<int>(intpartS[i])] + tableD[(static_cast<int>(intpartS[i]) + 1) % 4160] * fracpartS[i];
//		}
//
//		//apparently broken allpass interp.... try real allpass someday
//		
//
//		plerp<float, POLY>(buffer[0], buffer[2], mixX, outA);
//		plerp<float, POLY>(buffer[4], buffer[6], mixX, outB);
//		plerp<float, POLY>(outA, outB, mixY, outP);
//
//		plerp<float, POLY>(buffer[1], buffer[3], mixX, outC);
//		plerp<float, POLY>(buffer[5], buffer[7], mixX, outD);
//		plerp<float, POLY>(outC, outD, mixY, outS);
//
//		#pragma omp simd
//		for (int i = 0; i < POLY; i++) {
//			idxP[i] = fracpartP[i] + intpartP[i] + dIdxP[i];
//			idxS[i] = fracpartS[i] + intpartS[i] + dIdxS[i];
//			in_[i] = (outP[i] + outS[i]) / 2.0f;
//		}
//
//		filter.process(in_);
//		plerp<float, POLY>(*filter.lowpass(), *filter.highpass(), filterType, in_);
//		
//
//	}
//
//
//};



struct simplerWTableOsc {
private:
	const float slewMin = 0.1;
	const float slewMax = 10000.0;
	const float shapeScale = 1 / 10.0;
	const float minCutoff = 20.0f;
	const float maxCutoff = 10560.0f;

	StateVariableFilter filter;
	float filterType = 0.0f;

	float dt = 0.0f;
	float sampleRate = 0.0f;
	float dIdxP = 0.0f;
	float dIdxS = 0.0f;
	float idxP = 0.0f;
	float idxS = 0.0f;
	float freqMultiplier = 1.0f;
	float freq = 0.0f;
	float levelA = 0.0f;
	float levelB = 0.0f;

public:


	simplerWTableOsc(const float sampleRate_) {
		sampleRate = sampleRate_;
		dt = 1.0f / (sampleRate_ / static_cast<float>(4160));
		filterType = 0.0f;
		dIdxP = 0.0f;
		dIdxS = 0.0f;
		idxP = 0.0f;
		idxS = 0.0f;
		levelA = 0.0f;
		levelB = 0.0f;
		freqMultiplier = 1.0f;
		freq = 0.0f;

		filter.zero(sampleRate_);
		filter.setFreq(minCutoff);
	}

	void setFreq(const float hz_) {
		//cout << "Freq set to " << hz_ << " hz " << endl;
		freq = hz_;
	}

	//0-1
	void setFilterCutoff(const float c_) {
		filter.setFreq(minCutoff * powf(maxCutoff / minCutoff, c_));
	}

	void setFilterType(const float c_) {
		filterType = c_;
	}
	
	//0-1
	void setDetune(const float octave_) {
		const auto cents = octave_ * 1200.0f;
		freqMultiplier = roundf(100000.0f * powf(2.0f, (floorf(cents) / 100.0f / 12.0f))) / 100000.0f;
	}


	void setMix(const float levelA_, const float levelB_) {
		levelA = levelA_;
		levelB = levelB_;
	}


	float wav() {
		//apparently broken allpass interp.... try real allpass someday
		const auto sampA = interpvec<float>(tableA, idxP);
		const auto sampB = interpvec<float>(tableA, idxS);
		
		const auto sampC = interpvec<float>(tableB, idxP);
		const auto sampD = interpvec<float>(tableB, idxP);

		const float outA = sampA + sampB * 0.5f;
		const float outB = sampC + sampD * 0.5f;


		dIdxP = (freq)* dt;
		dIdxS = (freq * freqMultiplier) * dt;

		idxP += dIdxP;
		idxS += dIdxS;

		if (idxP > 4160 - 1) {
			idxP -= 4160 - 1;
		}

		if (idxS > 4160 - 1) {
			idxS -= 4160 - 1;
		}

		const float sample = (outA * levelA + outB * levelB) / max<float>(1.0f, levelA+levelB);
		filter.process(sample);
		
		return lerp<float>(filter.lowpass(), filter.highpass(), filterType);

	}

};





//struct qamOsc {
//private:
//	const float minCutoff = 20.0f;
//	const float maxCutoff = 10560.0f;
//
//	StateVariableFilter filter;
//	float filterType = 0.0f;
//
//	const array<float, 7> ratios = {
//		1.0f / 10.0f, 1.0f / 5.0f, 1.0f / 2.0f,	1.0f, 2.0f,	3.0f, 4.0f
//	};
//
//
//	float q_phase = 0.0f;
//	float m_phase = 0.0f;
//
//	float sampleRate = 1.0f;
//	float dt = 0.0f;
//	float freq = 0.0f;
//	float ratio = 0.428571f;
//
//
//public:
//
//
//	qamOsc(const float sampleRate_) {
//		sampleRate = sampleRate_;
//		dt = 1.0f / sampleRate;
//		filterType = 0.0f;
//		freq = 0.0f;
//		q_phase = 0.0f;
//		m_phase = 0.0f;
//		ratio = 0.428571f;
//		filter.zero(sampleRate);
//		filter.setFreq(minCutoff);
//	}
//
//	void setFreq(const float hz_) {
//		freq = hz_;
//	}
//
//	//0-1
//	void setFilterCutoff(const float c_) {
//		filter.setFreq(minCutoff * powf(maxCutoff / minCutoff, c_));
//	}
//	//0-1
//	void setFilterType(const float c_) {
//		filterType = c_;
//	}
//
//	//0-1
//	void setRatio(const float r_) {
//		ratio = r_;
//	}
//
//
//
//	float wav() {
//		const float dp_c = freq * dt;
//		const float dp_m = freq * interparray(ratios, ratio) * dt;
//		
//
//
//		q_phase += dp_c;
//		m_phase += dp_m;
//
//		if (q_phase > 1.0f) {
//			q_phase -= 1.0f;
//		}
//
//		if (m_phase > 1.0f) {
//			m_phase -= 1.0f;
//		}
//		
//		const float m = (sinf(2.0f * PI * m_phase) * 0.5f) + 1.0f;
//		//const float m =  sin(2 * PI * m_phase);
//		const float qs = sinf(2.0f * PI * q_phase);
//		const float qc = sinf(2.0f * PI * q_phase) * m;
//		const float sample = (qs + qc) * ISQRR2;
//		//const float sample = m;
//		filter.process(sample);
//
//		return lerp<float>(filter.lowpass(), filter.highpass(), filterType);
//
//	}
//
//
//};









