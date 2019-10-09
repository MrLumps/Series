#pragma once

#include "pffft.h"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>






using namespace std;

#define MAX_KERNEL 3145728
template<int blocksize>
struct IRKernel {
	size_t blockCount = 0;
	size_t inputBlock = 0;

	PFFFT_Setup* pffft = nullptr;
	float *kernelL = nullptr;
	float *kernelR = nullptr;
	float *inputFFTL = nullptr;
	float *inputFFTR = nullptr;

	array<float, blocksize * 2> inputWorkL;
	array<float, blocksize * 2> inputWorkR;
	array<float, blocksize * 2> outputWorkL;
	array<float, blocksize * 2> outputWorkR;
	array<float, blocksize> overlapL;
	array<float, blocksize> overlapR;
	array<float, blocksize * 2> tmp;

	//I guess since  we can't pre-process everything in the sampleManager
	//We'll just pre-allocate a big old chunk o data and gatekeep the sample assignment
	IRKernel() {
		pffft = pffft_new_setup(blocksize * 2, PFFFT_REAL);
		
		fill(inputWorkL.begin(), inputWorkL.end(), 0.0f);
		fill(inputWorkR.begin(), inputWorkR.end(), 0.0f);
		fill(outputWorkL.begin(), outputWorkL.end(), 0.0f);
		fill(outputWorkR.begin(), outputWorkR.end(), 0.0f);
		fill(overlapL.begin(), overlapL.end(), 0.0f);
		fill(overlapR.begin(), overlapR.end(), 0.0f);
		fill(tmp.begin(), tmp.end(), 0.0f);

		inputBlock = 0;

		blockCount = (MAX_KERNEL - 1) / blocksize + 1;

		kernelL = (float*)pffft_aligned_malloc(sizeof(float) * blocksize * 2 * blockCount);
		kernelR = (float*)pffft_aligned_malloc(sizeof(float) * blocksize * 2 * blockCount);
		inputFFTL = (float*)pffft_aligned_malloc(sizeof(float) * blocksize * 2 * blockCount);
		inputFFTR = (float*)pffft_aligned_malloc(sizeof(float) * blocksize * 2 * blockCount);

		blockCount = 0;
	}

	~IRKernel() {
		pffft_aligned_free(kernelL);
		pffft_aligned_free(kernelR);
		pffft_aligned_free(inputFFTL);
		pffft_aligned_free(inputFFTR);
		pffft_destroy_setup(pffft);
		kernelL = nullptr;
		kernelR = nullptr;
		inputFFTL = nullptr;
		inputFFTR = nullptr;
	}

	void loadSample(const shared_ptr<Sample>& sample) {
		assert(sample->size() < MAX_KERNEL);
		assert(sample->size() > 0);
	
		const int fftSize = (int)sample->size();

		fill(tmp.begin(), tmp.end(), 0.0f);

		blockCount = (fftSize - 1) / blocksize + 1;

		for (size_t b = 0; b < blockCount; b++) {

			const int copylen = mini(blocksize, fftSize - b * blocksize);

			std::fill(tmp.begin(), tmp.end(), 0.0f);
			memcpy(tmp.data(), &sample->getDataL()->data()[b * blocksize], sizeof(float) * copylen);
			pffft_transform(pffft, tmp.data(), &kernelL[blocksize * 2 * b], NULL, PFFFT_FORWARD);

			std::fill(tmp.begin(), tmp.end(), 0.0f);
			memcpy(tmp.data(), &sample->getDataR()->data()[b * blocksize], sizeof(float) * copylen);
			pffft_transform(pffft, tmp.data(), &kernelR[blocksize * 2 * b], NULL, PFFFT_FORWARD);

		}

		inputBlock = 0;

	}


	//input and output should be blockSize long
	void processBlock(vector<pair<float, float>> &in_, vector<pair<float, float>> &out_) {
			if (blockCount == 0) {
				std::fill(out_.begin(), out_.end(), make_pair(0.0f, 0.0f));
				return;
			}

			//Step input position (hmmmmmm)
			inputBlock = (inputBlock + 1) % blockCount;

			//Pad work blocks with zeros
			std::fill(inputWorkL.begin(), inputWorkL.end(), 0.0f);
			std::fill(inputWorkR.begin(), inputWorkR.end(), 0.0f);
			for(int i = 0; i < in_.size(); i++) {
				inputWorkL[i] = get<0>(in_[i]);
				inputWorkR[i] = get<1>(in_[i]);
			}

			// Compute input fft
			pffft_transform(pffft, inputWorkL.data(), &inputFFTL[blocksize * 2 * inputBlock], NULL, PFFFT_FORWARD);
			pffft_transform(pffft, inputWorkR.data(), &inputFFTR[blocksize * 2 * inputBlock], NULL, PFFFT_FORWARD);

			std::fill(outputWorkL.begin(), outputWorkL.end(), 0.0f);
			std::fill(outputWorkR.begin(), outputWorkR.end(), 0.0f);

			//std::cout << "Processing FFT Block" << std::endl;
			for (size_t b = 0; b < blockCount; b++) {
				const size_t pos = (inputBlock - b + blockCount) % blockCount; //This will rotate through the input FFT, I doubt I would have figured this out myself
																			   //std::cout << "Convolving kernel " << (blockSize * 2 * b) << " input: " << (blockSize * 2 * pos) << std::endl;
				pffft_zconvolve_accumulate(pffft, &kernelL[blocksize * 2 * b], &inputFFTL[blocksize * 2 * pos], outputWorkL.data(), 1.0);
				pffft_zconvolve_accumulate(pffft, &kernelR[blocksize * 2 * b], &inputFFTR[blocksize * 2 * pos], outputWorkR.data(), 1.0);
			}
			//std::cout << std::endl << std::endl << std::endl;

			// Transform accumulated fft back
			pffft_transform(pffft, outputWorkL.data(), outputWorkL.data(), NULL, PFFFT_BACKWARD);
			pffft_transform(pffft, outputWorkR.data(), outputWorkR.data(), NULL, PFFFT_BACKWARD);

			//Time domain overlap fix
			//see https://www.music.mcgill.ca/~gary/307/week11/ola.html
			for (size_t i = 0; i < blocksize; i++) {
				outputWorkL[i] += overlapL[i];
				outputWorkR[i] += overlapR[i];
			}

			const auto FFTscale = 1.0f / blocksize;
			for (size_t i = 0; i < blocksize; i++) {
				out_[i] = make_pair(outputWorkL[i] * FFTscale, outputWorkR[i] * FFTscale);
			}

			// Set overlap
			for (size_t i = 0; i < blocksize; i++) {
				overlapL[i] = outputWorkL[i + blocksize];
				overlapR[i] = outputWorkR[i + blocksize];
			}

	}


	inline int mini(int a, int b) {
		return a < b ? a : b;
	}

};