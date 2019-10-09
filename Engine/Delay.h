#pragma once

#include "Common.h"

using namespace std;

//
//Think I'm getting crakle when I modulate the delay to 0
//If this happens before a set operation
//Then it could be grabbing the tail end of the buffer
//rather then the current sample as expected
//Fix - Make sure 0 results in sample S-1 not current
// and if that is the case make sure it lands on S-2!
//
template <typename T, int S> struct StereoLerpLine {

private:
	T dataL[S];
	T dataR[S];
	int current = 0;
	const int end = S - 1;
	int delayL = end;
	int delayR = end;

public:

	//regular operation
	void set(pair<T, T> in_) {
		dataL[current] = in_.first;
		dataR[current] = in_.second;

		current++;
		if (current > end) {
			current = 0;
		}

	}

	// 0 - 1 where 1 is however long the delay line is in samples
	pair<T,T> get(const float delayL_, const float delayR_, const float attenuation_) {
		float intpartL;
		float intpartR;

		const float mixL = modf(delayL_ * end, &intpartL);
		delayL = static_cast<int>(intpartL);

		const float mixR = modf(delayR_ * end, &intpartR);
		delayR = static_cast<int>(intpartR);

		int indexL = current - delayL;
		if (indexL < 0) {
			indexL = end + indexL;
		}

		int indexR = current - delayR;
		if (indexR < 0) {
			indexR = end + indexR;
		}

		return make_pair<T, T>(
				lerp<T>(dataL[indexL], dataL[(indexL + 1) % end], mixL) * attenuation_,
				lerp<T>(dataR[indexR], dataR[(indexR + 1) % end], mixR) * attenuation_
			   );
			
	}


	///Ooops, remember to get rid of this one
	pair<T, T> get(const float delayL_, const float delayR_) {
		float intpartL;
		float intpartR;

		const float mixL = modf(delayL_ * end, &intpartL);
		delayL = static_cast<int>(intpartL);

		const float mixR = modf(delayR_ * end, &intpartR);
		delayR = static_cast<int>(intpartR);

		int indexL = current - delayL;
		if (indexL < 0) {
			indexL = end + indexL;
		}

		int indexR = current - delayR;
		if (indexR < 0) {
			indexR = end + indexR;
		}

		return make_pair<T, T>(
			lerp<T>(dataL[indexL], dataL[(indexL + 1) % end], mixL),
			lerp<T>(dataR[indexR], dataR[(indexR + 1) % end], mixR)
			);

	}

	int size() const {
		return S;
	}

	void zero() {
		std::fill_n(dataL, S, static_cast<T>(0));
		std::fill_n(dataR, S, static_cast<T>(0));
	}



};





template <typename T, int S> struct LerpLine {

private:
	T data[S];
	const int end = S - 1;
	int current = 0;
	int delay = end;
	float intpart = 0.0f;
	float mix = 0.0f;

public:
	//regular operation
	void set(const float s_) {
		data[current] = s_;

		current++;
		if (current > end) {
			current = 0;
		}
	
	}

	void set(const float s_, const float fb_) {
		data[current] = s_ + data[current] * fb_;

		current++;
		if (current > end) {
			current = 0;
		}
	}

	//Write to offset
	void write(const int offset, const float s_) {
		data[wrap(current - offset)] = s_;
	}

	void write(const int offset, const float s_, const float fb_) {
		data[wrap(current - offset)] = s_ + data[wrap(current - offset)] * fb_;
	}

	int wrap(const int idx_) {
		//return (idx_ < 0 ? idx_ + S : idx_);
		return (idx_ < 0 ? idx_ + S : idx_);
	}

	// 0 - 1 where 1 is however long the delay line is in samples
	float tap(const float delay_) {
		mix = modf(delay_ * end, &intpart);
		delay = static_cast<size_t>(intpart);

		//There must be some way to do this neater
		//but I don't care for now if I get rid of that
		//off by one error
		const int index_a = wrap(current - delay);
		const int index_b = wrap(index_a - 1);

		return lerp<T>(data[index_a], data[index_b], mix);
	}

	float tap(const int offset, const float mix_) {
		const int index_a = wrap(current - offset);
		const int index_b = wrap(index_a - 1);

		return lerp<T>(data[index_a], data[index_b], mix);
	}

	float tap(const int offset) {
		const int index_a = wrap(current - offset);
		return data[index_a];
	}

	int size() const {
		return S;
	}

	void zero() {
		std::fill_n(data, S, static_cast<T>(0));
	}



};


























////
////fix this one 1st
////
//template <typename T, int S> struct LerpLine {
//
//private:
//	T data[S];
//	const int end = S - 1;
//	int current = 0;
//	int delay = end;
//	float mix = 0.0f;
//
//public:
//
//	float get() {
//		int index = current - delay;
//		if (index < 0) {
//			index = end + index;
//		}
//
//		return lerp<T>(data[index], data[index + 1], mix);
//	}
//
//	//regular operation
//	void set(const float t_) {
//		current++;
//		if (current > end) {
//			current = 0;
//		}
//
//		data[current] = t_;
//	}
//
//	//Write at a particular offset
//	void set(const int o_, const float t_) {
//		int index = current - o_;
//		if (index < 0) {
//			index = end + index;
//		}
//
//		data[index] = t_;
//	}
//
//	// 0 - 1 where 1 is however long the delay line is in samples
//	float tap(const float delay_) {
//		setDelay(delay_);
//		return get();
//	}
//
//	float tap(const int d_) {
//		int index = current - d_;
//		if (index < 0) {
//			index = end + index;
//		}
//
//		return data[index];
//	}
//
//
//	// 0 - 1 where 1 is however long the delay line is in samples
//	void setDelay(const float delay_) {
//		float intpart;
//		mix = modf(delay_ * end, &intpart);
//		delay = static_cast<size_t>(intpart);
//	}
//
//	// 0 - S in samples, input not range checked
//	void setDelay(const int delay_) {
//		mix = 0;
//		delay = delay_;
//	}
//
//	// 0 - S in samples, input not range checked
//	//Mix is the amount of S+1 to mix in
//	void setDelay(const int delay_, const float mix_) {
//		mix = mix_;
//		delay = delay_;
//	}
//
//
//
//	int size() const {
//		return S;
//	}
//
//	void zero() {
//		std::fill_n(data, S, static_cast<T>(0));
//	}
//
//
//
//};




//A beefed up ringbuffer
template <typename T, int S> struct DelayLine {

private:
	const int end = S - 1;
	int current = 0;
	int delay = end;
	T data[S];
	

public:

	float get() {
		int index = current - delay;
		if (index < 0) {
			index = end + index;
		}

		return data[index];
	}

	float tap(const int delay_) {
		int index = current - delay_;
		if (index < 0) {
			index = end + index;
		}

		return data[index];
	}

	//regular operation
	void set(const float t) {
		data[current] = t;

		current++;
		if (current > end) {
			current = 0;
		}

	}

	// 0 - 1 where 1 is however long the delay line is in samples
	void setDelay(const float delay_) {
		delay = static_cast<size_t>(delay_ * (S - 1));
	}

	// 0 - S in samples, input not range checked
	void setDelay(const int delay_) {
		delay = delay_;
	}

	int size() const {
		return S;
	}

	//Diagnostics
	int getCurrent() {
		return current;
	}

	void zero() {
		std::fill_n(data, S, static_cast<T>(0));
	}

};