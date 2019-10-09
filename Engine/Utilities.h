#pragma once

#include <cstdint>





//Move elsewhere
// Improved Rng see numerical recipies and wikipedia
struct Ran {
	uint_fast64_t u, v, w;

	Ran() {
		Ran(738); // very random
	}

	//Call with int seed except v below
	Ran(uint_fast64_t j) : v(4101842887655102017LL), w(1) {
		u = j ^ v; int64();
		v = u; int64();
		w = v; int64();
	}

	inline uint_fast64_t int64() {
		u = u * 2862933555777941757LL + 7046029254386353087LL;
		v ^= v >> 17;
		v ^= v << 31;
		v ^= v >> 8;
		w = 4294957665U * (w & 0xffffffff) + (w >> 32);
		uint_fast64_t x = u ^ (u << 21);
		x ^= x >> 35;
		x ^= x << 4;
		return (x + v) ^ w;
	}

	//returns 0 to 1
	inline double doub() { return 5.42101086242752217E-20 * int64(); }
	//float
	inline float flt() { return (float)doub(); }
	//32bit int
	inline uint32_t int32() { return (uint32_t)int64(); }
	//8bit
	inline uint8_t int8() { return (uint8_t)int64(); }
	//bool
	inline bool bit() { return bool(int64() & (1 << 0)); }

};


#define PINK_MAX_RANDOM_ROWS   (30)
#define PINK_RANDOM_BITS       (24)
#define PINK_RANDOM_SHIFT      ((sizeof(long)*8)-PINK_RANDOM_BITS)


typedef struct
{
	long      pink_Rows[PINK_MAX_RANDOM_ROWS];
	long      pink_RunningSum;   /* Used to optimize summing of generators. */
	int       pink_Index;        /* Incremented each sample. */
	int       pink_IndexMask;    /* Index wrapped by ANDing with this mask. */
	float     pink_Scalar;       /* Used to scale within range of -1.0 to +1.0 */
} PinkNoise;

/* Setup PinkNoise structure for N rows of generators. */
void InitializePinkNoise(PinkNoise* pink, int numRows)
{
	int i;
	long pmax;
	pink->pink_Index = 0;
	pink->pink_IndexMask = (1 << numRows) - 1;
	/* Calculate maximum possible signed random value. Extra 1 for white noise always added. */
	pmax = (numRows + 1) * (1 << (PINK_RANDOM_BITS - 1));
	pink->pink_Scalar = 1.0f / pmax;
	/* Initialize rows. */
	for (i = 0; i < numRows; i++) pink->pink_Rows[i] = 0;
	pink->pink_RunningSum = 0;
}

/* Generate Pink noise values between -1.0 and +1.0 */
float GeneratePinkNoise(PinkNoise* pink, Ran& rng)
{
	long newRandom;

	/* Increment and mask index. */
	pink->pink_Index = (pink->pink_Index + 1) & pink->pink_IndexMask;

	/* If index is zero, don't update any random values. */
	if (pink->pink_Index != 0)
	{
		/* Determine how many trailing zeros in PinkIndex. */
		/* This algorithm will hang if n==0 so test first. */
		int numZeros = 0;
		int n = pink->pink_Index;
		while ((n & 1) == 0)
		{
			n = n >> 1;
			numZeros++;
		}

		/* Replace the indexed ROWS random value.
		* Subtract and add back to RunningSum instead of adding all the random
		* values together. Only one changes each time.
		*/
		pink->pink_RunningSum -= pink->pink_Rows[numZeros];
		newRandom = ((long)rng.int32()) >> PINK_RANDOM_SHIFT;
		pink->pink_RunningSum += newRandom;
		pink->pink_Rows[numZeros] = newRandom;
	}

	/* Add extra white noise value. */
	newRandom = ((long)rng.int32()) >> PINK_RANDOM_SHIFT;
	const long sum = pink->pink_RunningSum + newRandom;

	/* Scale to range of -1.0 to 0.9999. */
	return pink->pink_Scalar * sum;
}