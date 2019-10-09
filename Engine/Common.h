#pragma once
#include <utility>
#include <array>

constexpr auto AUDIO_BLOCK_SIZE = (256);
constexpr auto POLY = (4);
constexpr auto CHANNELS = (8);
constexpr auto MAX_SFX_PB = (4);



//sigh
constexpr float PI = (3.14159265358979323846f);
constexpr float SQRR2 = (1.4142135623730950488016887242097f);
constexpr float ISQRR2 = (1.0f/SQRR2);
constexpr float LN2 = (0.69314718056f);




inline float aproxSine(const float p_) {
	float s = p_ * 0.15915f;
	s = s - static_cast<int>(s);
	return 20.785f * s * (s - 0.5f) * (s - 1.0f);
}



// 0 - 1
inline float panL(const float pan_) {
	return cosf(PI * (pan_) / 2.0f);
}

// 0 - 1
inline float panR(const float pan_) {
	return sinf(PI * (pan_) / 2.0f);
}

inline float clamp(const float x_, const float min_, const float max_) {
	return fmaxf(fminf(x_, fmaxf(min_, max_)), fminf(min_, max_));
}

template <typename T> inline T lerp(const T a_, const T b_, const T m_) {
	return ((1 - m_) * a_) + (m_ * b_);
}

template <typename T> inline T crossfade(const T a_, const T b_, const T m_) {
	return a_ + m_ * (b_ - a_);
}

//pair<float, float> inline slerp(const pair<float, float> a_, const pair<float, float> b_, const float m_) {
//	//lerp from
//	return make_pair<float, float>((1.0f - m_) * get<0>(a_) + m_ * get<0>(b_), (1.0f - m_) * get<1>(a_) + m_ * get<1>(b_));
//}


template<typename T, int Poly>
inline void plerp(const std::array<T, Poly> a_, const std::array<T, Poly> b_, const std::array<T, Poly> mix_, std::array<T, Poly>& result_) {
#pragma omp simd
	for (int i = 0; i < Poly; i++) {
		result_[i] = (1 - mix_[i]) * a_[i] + mix_[i] * b_[i];
	}
}

template <typename T> inline T rescale(const T x_, const T xMin_, const T xMax_, const T yMin_, const T yMax_) {
	return yMin_ + (x_ - xMin_) / (xMax_ - xMin_) * (yMax_ - yMin_);
}
//
//template <typename T> inline int sgn(T val) {
//	return val ? (T(0) < val) - (val < T(0)) : 1;
//}

//array index 0-1, it will interpralate a value out of that array
template<typename T, int S> T interparray(const std::array<T, S> & data, const float index_) {
	const int idx = static_cast<int>(index_);
	return lerp<T>(data[idx], data[(idx + 1) % (S - 1)], index_ - idx);
};

template<typename T> T interpvec(const std::vector<T>& data, const float index_) {
	const int idx = static_cast<int>(index_);
	return lerp<T>(data[idx], data[((size_t)idx + 1) % (data.size()-1)], index_ - idx);
};


//*Yoink* 
//Pharap @ https://stackoverflow.com/questions/1903954/is-there-a-standard-sign-function-signum-sgn-in-c-c
template <typename T> inline constexpr
int signum(T x, std::false_type is_signed) {
	return T(0) < x;
}

template <typename T> inline constexpr
int signum(T x, std::true_type is_signed) {
	return (T(0) < x) - (x < T(0));
}

template <typename T> inline constexpr
int signum(T x) {
	return signum(x, std::is_signed<T>());
}