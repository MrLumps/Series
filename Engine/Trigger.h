#pragma once


struct Trigger {

	enum States {
		UNSET,
		LOW,
		HIGH
	};

	States state = States::UNSET;
	const float lowThreshold = 0.0f;
	const float highThreshold = 0.1f;

	bool process(const float in_) {
		switch (state) {
		case States::LOW:
			if (in_ >= highThreshold) {
				state = States::HIGH;
				return true;
			}
			break;
		case States::HIGH:
			if (in_ <= lowThreshold) {
				state = States::LOW;
			}
			break;
		case States::UNSET:
			if (in_ >= highThreshold) {
				state = States::HIGH;
			}
			else if (in_ <= lowThreshold) {
				state = States::LOW;
			}
			break;
		}
		return false;
	}

};


struct TimedPulse {
	float currentTime = 0.0f;
	float pulseLength = 0.0f;

	//Time in seconds
	void trigger(const float l_) {
		if (l_ + pulseLength > pulseLength) {
			pulseLength = l_;
			currentTime = 0.0f;
		}
	}

	bool process(const float dt_) {
		currentTime += dt_;
		return currentTime < pulseLength;
	}

	void zero() {
		currentTime = 0.0f;
		pulseLength = 0.0f;
	}

};