#pragma once


struct Adsr {
	const float maxLength = 20.0f;

	float sampleRate = 44100.0f;

	float attack = 0.5;
	float decay = 0.5;
	float sustain = 0.5;
	float release = 0.5;

	bool decaying = false;
	float state = 0.0f;

	Adsr(const float sampleRate_) {
		sampleRate = sampleRate_;
	}

	
	float process(const float in_) {

		if (in_ >= 1.0f) {
			if (decaying) {
				// Decay
				if (release < 1e-4) {
					state = sustain;
				}	else {
					state += powf(20000.0f, 1.0f - decay) / maxLength * (sustain - state) / sampleRate;
				}
			}
			else {
				// Attack
				// Skip ahead if attack is all the way down (infinitely fast)
				if (attack < 1e-4) {
					state = 1.0;
				}
				else {
					state += powf(20000.0f, 1.0f - attack) / maxLength * (1.01f - state) / sampleRate;
				}
				if (state >= 1.0) {
					state = 1.0;
					decaying = true;
				}
			}
		}
		else {
			// Release
			if (release < 1e-4) {
				state = 0.0;
			}
			else {
				state += powf(20000.0f, 1.0f - release) / maxLength * (0.0f - state) / sampleRate;
			}
			decaying = false;
		}

		return state;
	}



	void reset() {
		attack = 0.5;
		decay = 0.5;
		sustain = 0.5;
		release = 0.5;

		decaying = false;
		state = 0.0f;
	}


};


struct TriggeredEnvelope {
	float state = 0.0f; //0 - 1
	bool rising = false;
	bool decaying = false;

public:
	float attack = 0.0f; //time in ms
	float sustain = 0.0f; //level 0 - N, though 1 or 10 would be normal
	float release = 0.0f; //time in ms
	bool triggerMode = false;

	void zero() {
		state = 0.0f; //0 - sustain
		rising = false;
		decaying = false;
		attack = 0.0f; //time in ms
		sustain = 0.0f; //level 0 - N, though 1 or 10 would be normal
		release = 0.0f; //time in ms
		triggerMode = false;
	}

	void trigger() {
		rising = true;
		decaying = false;
	}


	void step(const float dt_) {

		if (rising) {
			state += sustain / (((0.000001f + attack) / 1000.0f) / dt_);

			if (state >= sustain) {
				state = sustain;
				rising = false;
				decaying = true;
			}

		}

		if (decaying) {
			state -= sustain / (((0.000001f + release) / 1000.0f) / dt_);

			if (state < 1e-4) {
				state = 0.0f;
				decaying = false;
			}

		}

	}

	float level() {
		return state;
	}

};

struct AR {
private:
	bool start = false;
	bool end = false;
	bool isaccent = false;
	float state = false;
	float sampleRate = 44100.0f;

public:

	float maxReleaseTime = 2.0f;
	float maxAttackTime = 0.1f;
	float attack = 0.0f; // 0 - 1
	float release = 0.5f;	 // 0.5 - 1
	float level = 1.0f; // 1 - 0

	AR(const float attack, const float release) : attack(attack), release(release) { state = 0.0f; start = false; }
	AR() { attack = 0.0f; release = 0.5f; state = 0.0f; level = 1.0f;  start = false; }

	void setSampleRate(const float sampleRate_) {
		sampleRate = sampleRate_;
	}

	const void inline trigger() {
		start = true;
		isaccent = false;
	}

	const void inline kill() {
		end = true;
	}

	const void inline accent() {
		isaccent = true;
	}

	const bool inline isAccent() {
		return isaccent;
	}

	float env() {
		return state;
	}


	float step() {

		if (end) {

			if (state < 1e-4) {
				state = 0.0f;
				end = false;
				isaccent = false;
			}

			//0.522f fudged by ear
			if (state) {
				state += powf(sampleRate / 2.0f, 1.0f - 0.522f) / maxReleaseTime * (0.0f - state) / sampleRate;
			}


		}
		else if (start) {
			state += powf(sampleRate / 2.0f, 1.0f - attack) / maxAttackTime * (1.01f - state) / sampleRate;

			if (state >= level) {
				state = level;
				start = false;
			}

		}
		else {

			if (state) {
				state += powf(sampleRate / 2.0f, 1.0f - release) / maxReleaseTime * (0.0f - state) / sampleRate;

				if (state < 1e-4) {
					state = 0.0f;
					isaccent = false;
				}
			}

		}

		return state;

	}

};








