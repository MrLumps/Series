#pragma once
#define SEQUENCE_MAX 256
#include <cstdint>


struct bitBucket {
	const uint64_t a;
	const uint64_t b;
	const uint64_t c;
	const uint64_t d;
};


struct patternBucket {
	const bitBucket* data;
	patternBucket(const bitBucket *data) : data(data) {};

	inline bool operator[](size_t pos) const
	{
		switch (pos / 64) {
		case 0:
			return ((data->a & ((uint64_t)1 << pos)));
		case 1:
			return ((data->b & ((uint64_t)1 << (pos - 64))));
		case 2:
			return ((data->c & ((uint64_t)1 << (pos - 128))));
		case 3:
			return ((data->d & ((uint64_t)1 << (pos - 192))));
		default:
			return 0;
		}
	}

};
