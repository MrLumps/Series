#pragma once

#include "Utilities.h"




// Stepper Base
struct Stepper {
public:
	enum Modes {
		FORWARD_MODE,
		REVERSE_MODE,
		BOUNCE_MODE,
		RANDOM_MODE,
		NUM_MODES
	};

	size_t length = 1; //I guess I could make this totally safe by changing it to uint16_t
	bool endOfCycle = false;

protected:
	Ran rng;
	int currentStep = 0;
	int rngSteps = 0;
	Modes currentMode = Modes::FORWARD_MODE;
	bool bounce = false;

	void inline AdvanceStep() {
		endOfCycle = false;

		switch (currentMode) {

		case Modes::FORWARD_MODE:
			currentStep++;
			if (currentStep >= length) {
				currentStep = 0;
				endOfCycle = true;
			}
			break;

		case Modes::REVERSE_MODE:
			currentStep--;
			if (currentStep < 0) {
				currentStep = std::max<int>(length - 1, 0);
				endOfCycle = true;
			}
			break;

		case Modes::BOUNCE_MODE:
			if (bounce) {
				currentStep--;
				if (currentStep <= 0) {
					currentStep = 0;
					bounce = !bounce;
					endOfCycle = true;
				}

			}
			else {
				currentStep++;
				if (currentStep >= length - 1) {
					currentStep = std::max<int>(length - 1, 0);
					bounce = !bounce;
				}

			}
			break;

		case Modes::RANDOM_MODE:
			currentStep = length ? rng.int64() % length : 0;
			rngSteps++;
			if (rngSteps >= length) {
				rngSteps = 0;
				endOfCycle = true;
			}
			break;
		}

	}

public:
	void inline CycleMode() {
		
		currentMode = static_cast<Modes>(static_cast<int>(currentMode) + 1);
		if (currentMode >= Modes::NUM_MODES) {
			currentMode = Modes::FORWARD_MODE;
		}
		rngSteps = 0;
	}

	int inline GetMode() {
		return static_cast<int>(currentMode);
	}

	void SetMode(const int m_) {
		currentMode = static_cast<Modes>(m_);
	}

};




struct SwitchStepper : Stepper {
public:
	int getCurrentStep() {
		return currentStep;
	}

	void Step() {
		AdvanceStep();
	}

};

