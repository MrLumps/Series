#pragma once

#include <memory>
#include <list>
#include <vector>
#include <array>
#include <iterator>
#include <algorithm>
#include <fstream>



#include "BaseObjects.h"
#include "ModMatrix.h"
#include "Stepper.h"
#include "Measures.h"
#include "Kits.h"
#include "Rules.h"

#include "erBitData.h"

using namespace std;
#define SEQUENCE_MAX 256


//Container for Measure objects
//Allows routing to channels
//Via modBlock


//
//Create a default measure for user triggers
//

struct StepperBase {
	int pauseSteps = 0;
	bool eoc = false;
	bool triggered = false;
	virtual float process(const float in_) { return 0.0f; }
	virtual void restart() { ; }
	virtual void jog(const int steps_) { ; }
	void pause(const int steps_) { pauseSteps = steps_; }
	virtual void clear() { pauseSteps = 0; eoc = false; triggered = false; }
	virtual json toConfig() {
		json config;
		return config;
	}
	virtual void loadConfig(const json cfg) { ; }

};






struct EuclidianDivider : StepperBase {
private:

	Ran rng;
	const float threshold = 1.0f;

	unsigned int currentStep = 0;
	bitBucket* bucket;

	bool coinFlip = false;

	float last = 0.0f;
	float output = 0.0f;

	

	// Given the current step, fill, length members and the given probablility
	bool SetNote(const float p, const float rnd) {
		bool noteOn = false;

		if (fill > 0) {
			// Flip coin
			if (p < 0.999f) {
				if (rnd <= p) {
					coinFlip = true;
				}
			}
			// Normal operations
			if (coinFlip == false) {

				if (fill < length) {
					const patternBucket pattern_ref(&bucket[((fill * (SEQUENCE_MAX + 1)) + length)]);
					if (pattern_ref[currentStep]) {
						noteOn = true;
					}
				}
				else if (coinFlip == false) {  // if fill > length output a gate
					noteOn = true;
				}
			}
		}

		return noteOn;
	}

	bool AdvanceStep() {
		coinFlip = false;
		currentStep++;
		if (currentStep + 1 > length) {
			currentStep = 0;
			return true;
		}

		return false;
	}








public:
	int fill = 0;
	size_t length = 0;
	float chance = 1.0f;


	EuclidianDivider(bitBucket* bucket_) {
		bucket = bucket_;
	}

	//float process(const float in_) {
	float process(const float in_) {

		bool rising = false;
		bool falling = false;
		triggered = false;
		eoc = false;

		if (pauseSteps > 0) {
			if (in_ >= threshold) {
				if (last < threshold) {
					rising = true;
					pauseSteps--;
				}
			}

			if (in_ < threshold) {
				if (last >= threshold) {
					falling = true;
				}
			}

			return 0.0f;
		}

		if (in_ >= threshold) {
			if (last < threshold) {
				rising = true;
				output = SetNote(chance, rng.flt()) ? 1.0f : 0.0f;
				if (output)
					triggered = true;
			}
		}

		if (in_ < threshold) {
			if (last >= threshold) {
				falling = true;
				eoc = AdvanceStep();
				output = 0.0f;
			}
		}


		last = in_;
		return output;

	}

	void restart() {
		currentStep = 0;
	}

	void clear() { 
		pauseSteps = 0; 
		eoc = false; 
		triggered = false; 
		fill = 0;
		length = 0;
		chance = 1.0f;
		currentStep = 0;
		coinFlip = false;
		last = 0.0f;
		output = 0.0f;
	}


	void jog(const int steps_) {
		if (length > 0)
			currentStep = (steps_ + currentStep) % length;
	}

	json toConfig() {
		json config;
		config["fill"] = fill;
		config["length"] = length;
		config["chance"] = chance;
		return config;
	}
	void loadConfig(const json cfg) {
		if (cfg.count("fill"))
			fill = cfg["fill"].get<int>();
		if (cfg.count("length"))
			length = cfg["length"].get<int>();
		if (cfg.count("chance"))
			chance = cfg["chance"].get<float>();
	}
};



struct StepPattern : Stepper {
public:

	enum Events {
		NONE,
		TRIGGER,
		ACCENT,
		RETRIGGER,
		EVENTS
	};

	float probability;
	vector<int> pattern;

	StepPattern() {
		length = 16;
		pattern.reserve(256);
		pattern.resize(16, 0);
		Reset();
	}

	void Reset() {
		for (auto& p : pattern) {
			p = Events::NONE;
		}

		fill(pattern.begin(), pattern.end(), 0);

		currentStep = 0;
		probability = 1.0f;
		bounce = false;
		currentMode = Modes::FORWARD_MODE;
	}

	void Restart() {
		currentStep = 0;
	}

	void inline CycleEvent(const size_t pos_) {
		//pattern[pos_] = (pattern[pos_] + 1) % Events::EVENTS;
		pattern[pos_] = !pattern[pos_];
	}

	int getCurrentStep() {
		return currentStep;
	}

	void clear() {
		length = 16;
		for (auto& v : pattern) {
			v = 0;
		}
	}

	void jog(const int steps_) {
		currentStep = (currentStep + steps_) % length;
	}

	inline bool getEvent() {
		if (pattern[currentStep] > 0) {
			if (probability < 0.999f) {
				if (rng.flt() <= 1.0f - probability) {
					return false;
				}
			}
			return true;
		}
		return false;
	}



	bool inline Step() {
		AdvanceStep();

		return endOfCycle;
	}

};


//Hmm 24 ppqn
//1 bar = 96 pulses
//Not terrific for odd time signatures
//Though can do tripplets easily
//Hmm 840 can do a lot

struct StepSequence : StepperBase {

private:


	Trigger trigger;
	float chance = 1.0f;

public:
	StepPattern pattern;

	StepSequence() {
		pattern.Reset();
	}

	float process(const float in_) {
		eoc = false;

		if (trigger.process(in_)) {
			if (pauseSteps > 0) {
				pauseSteps--;
				return 0.0f;
			}
			eoc = pattern.Step();
			if (pattern.getEvent())
				triggered = true;
		}

		return (in_ && pattern.getEvent()) ? 1.0f : 0.0f;

	}

	void restart() {
		pattern.Restart();
	}

	void clear() {
		pauseSteps = 0;
		eoc = false;
		triggered = false;
		chance = 1.0f;
		pattern.clear();
	}

	void jog(const int steps_) {
		pattern.jog(steps_);
	}

	json toConfig() {
		json config;
		config["length"] = pattern.length;
		config["probability"] = pattern.probability;
		config["pattern"] = pattern.pattern;
		return config;
	}
	void loadConfig(const json cfg) {
		if(cfg.count("length"))
			pattern.length = cfg["length"].get<int>();
		if (cfg.count("probability"))
			pattern.probability = cfg["probability"].get<float>();
		pattern.pattern.clear();
		if (cfg.count("pattern"))
			for (auto& v : cfg["pattern"]) {
				pattern.pattern.push_back(v.get<int>());
			}
	}

};






struct ClockProvider {
	enum class ClockTypes {
		NONE,
		WHOLE,
		HALF,
		TRIPPLET,
		QUARTER,
		EIGHTH,
		DOUBLE,
		TRIPPLE,
		QUADRUPLE
	};

private:
	//While I would like to use the following
	//const vector<int> pulseCounts = {96, 48, 32, 24, 12, 6};
	//currently to drive grids with the master clock we need
	const array<int, 9> pulseCounts { 0, 24, 12, 8, 6, 3, 48, 72, 96 };
	array<int, 6> pulseCount;
	array<int, 6> pulseTarget;
	array<bool, 6> flip;


public:
	array<float, 6> gateData;
	array<bool, 6> tickData;
	float swing = 0.0f;

	ClockProvider() {
		pulseCount.fill(0);
		pulseTarget.fill(0);
		flip.fill(false);
		gateData.fill(0.0f);
		tickData.fill(0.0f);
		swing = 0.0f;
	}

	void process(const bool tick_) {

		for (int i = 0; i < 6; i++) {
			pulseCount[i] += static_cast<int>(tick_);

			if (pulseCount[i] >= pulseTarget[i]) {
				pulseCount[i] = 0;
				tickData[i] = true;

				const int swingMax = pulseCounts[i] / 2;
				const int swingAmount = static_cast<int>(swing * swingMax);
				if (!flip[i])
					pulseTarget[i] = pulseCounts[i] + swingAmount;
				if (flip[i])
					pulseTarget[i] = pulseCounts[i] - swingAmount;

				flip[i] = !flip[i];
			}
			else {
				tickData[i] = false;
			}

			gateData[i] = (pulseCount[i] < pulseTarget[i] / 2) ? 1.0f : 0.0f;

		}

	}

};












struct EmptySequencer {

	array<float, POLY> gates;

	EmptySequencer() {
		for (auto& v : gates) {
			v = 0.0f;
		}

	}

	void process(const float clkin) {
		;
	}

	virtual json toConfig() {
		json config;

		for (auto& v : gates) {
			config.push_back(v);
		}

		return config;
	}

	void loadConfig(const json cfg) {

		for (int i = 0; i < gates.size(); i++) {
			gates[i] = cfg.get<float>();
		}

	}

};


















struct StepSequencer {
	enum class Events {
		SEQUENCE_TRIGGER,
		SEQUENCE_END_OF_CYCLE,
		CLOCK
	};

	enum class Actions {
		SWAP,			//Swap laneList lane action_src to action_dst
		ROTATE_FWD,		//Roates the lane list from action_src to action_dst by 1
		ROTATE_BCK,		//Roates the lane list from action_src to action_dst by 1
		CHANGE_CLOCK,	//Changes the clock in lane action_src to clock type action_dst
		JOG,			//Advances the stepper in lane action_src action_dst steps
		PAUSE,			//Pauses the stepper in lane action_src action_dst steps
		RANDOMIZE,		//Swaps the stepper in lane action_src with random lane
		RESET,			//Reset step sequencer to step 0 and clears rule counters
		ENABLE_RULE,	//Enable rule indexed in action_src
		DISABLE_RULE,   //Disable rule indexed in action_src
		TOGGLE_RULE		//Toggles rule indexed in action_src
	};

	enum class Types {
		NONE,
		STEP,
		EUCLIDIAN
	};


	struct StepperLane {
		//int id = -1;
		Types type = Types::NONE;
		ClockProvider::ClockTypes clockType = ClockProvider::ClockTypes::NONE;
		bool triggered = false;
		bool eoc = false;
	};

	Ran rng;
	unique_ptr<ClockProvider> clockProvider;  //fine
	bitBucket* bucket = nullptr;

	shared_ptr<StepperBase> emptyStepper;
	array< shared_ptr<StepSequence>, POLY> stepSequencers;
	array< shared_ptr<EuclidianDivider>, POLY> euclidianDividers;
	
	array<float, POLY> gates;									
	array<StepperLane, POLY> lanes;
	vector<int> laneMapper;
	vector<Rule<Events, Actions>> rules;


	Types getLaneType(const int lane_) {
		return lanes[lane_].type;
	}

	int getMappedLane(const int laneIdx_) {
		//return *next(laneMapper.begin(), laneIdx_);
		return laneMapper[laneIdx_];
	}

	//0..7
	void setLaneType(const int lane_, const Types t_) {

		if (lanes[lane_].type != t_) {
			lanes[lane_].type = t_;
		}

	}

	/*
			TRIGGER,
		ACCENT,
		RETRIGGER,
	*/


	void toggleCurrentStep(const int lane_) {
		if (getLaneType(lane_) == Types::STEP) {
			const auto currentStep = stepSequencers[lane_]->pattern.getCurrentStep();
			if (stepSequencers[lane_]->pattern.pattern[currentStep] > 0) {
				stepSequencers[lane_]->pattern.pattern[currentStep] = 0;
			}
			else {
				stepSequencers[lane_]->pattern.pattern[currentStep] = 1;
			}
		}
	}




	ClockProvider::ClockTypes getLaneClockType(const int lane_) {
		return lanes[lane_].clockType;
	}

	void setLaneClockType(const int lane_, const ClockProvider::ClockTypes t_) {
		lanes[lane_].clockType = t_;
	}

	shared_ptr<StepperBase> getLaneStepper(const int lane_) {
		//return lanes[lane_].stepper;
		switch (lanes[lane_].type) {
		default:
			return emptyStepper;
		case Types::STEP:
			return static_pointer_cast<StepperBase>(stepSequencers[lane_]);
		case Types::EUCLIDIAN:
			return static_pointer_cast<StepperBase>(euclidianDividers[lane_]);
	
		}
	}



	void restart() {
		for (auto& p : stepSequencers) {
			p->restart();
		}
		for (auto& p : euclidianDividers) {
			p->restart();
		}
	}

	StepSequencer(bitBucket* bucket_) {
		bucket = bucket_;
		clockProvider = make_unique<ClockProvider>();

		emptyStepper = make_shared<StepperBase>();
		for (auto& p : stepSequencers) {
			p = make_shared<StepSequence>();
		}
				
		for (auto& p : euclidianDividers) {
			p = make_shared<EuclidianDivider>(bucket);
		}

		laneMapper.reserve(POLY); // poly
		int i = 0;
		for (auto& l : lanes) {
			l.type = Types::NONE;
			l.clockType = ClockProvider::ClockTypes::NONE;
			l.triggered = false;
			l.eoc = false;
			laneMapper.push_back(i);
			i++;
		}

		for (auto& v : gates) {
			v = 0.0f;
		}

		rules.reserve(32);

	}

	void clear() {
		for (auto& p : stepSequencers) {
			p->clear();
		}

		for (auto& p : euclidianDividers) {
			p->clear();
		}

		int i = 0;
		laneMapper.clear();
		for (auto& l : lanes) {
			l.type = Types::NONE;
			l.clockType = ClockProvider::ClockTypes::NONE;
			l.triggered = false;
			l.eoc = false;
			laneMapper.push_back(i);
			i++;
		}
		rules.clear();

	}


	//bit of a hack, this should probably be in step sequencer
	size_t maxStepLength() {
		size_t m = 0;

		for (int i = 0; i < POLY; i++) { // poly
			
			if (lanes[i].type == StepSequencer::Types::STEP) {
				if (stepSequencers[i]->pattern.length > m) {
					m = stepSequencers[i]->pattern.length;
				}
			}


		}

		return m;
	}

	   	  
	void process(const float clkin) {
		clockProvider->process(bool(clkin));

		for (auto& r : rules) {
			if (r.run) {
				if (r.event_counter >= r.action_threashold) {
					switch (r.action) {
					case Actions::SWAP:
						iter_swap(next(laneMapper.begin(), r.action_src), next(laneMapper.begin(), r.action_dst));
						break;
					case Actions::ROTATE_FWD:
						rotate(next(laneMapper.begin(), r.action_src), next(laneMapper.begin(), (long long)r.action_src + 1), next(laneMapper.begin(), (long long)r.action_dst+1));
						break;
					case Actions::ROTATE_BCK:
						rotate(next(laneMapper.begin(), r.action_src), next(laneMapper.begin(), (long long)r.action_dst - 1), next(laneMapper.begin(), (long long)r.action_dst+1));
						break;
					case Actions::CHANGE_CLOCK:
						setLaneClockType(r.action_src, static_cast<ClockProvider::ClockTypes>(r.action_dst));
						break;
					case Actions::JOG:
					{
						auto stepper = getLaneStepper(r.action_src);
						if (stepper)
							stepper->jog(r.action_dst);
					}
					break;
					case Actions::PAUSE:
					{
						auto stepper = getLaneStepper(r.action_src);
						if (stepper)
							stepper->pause(r.action_dst);
					}
					break;
					case Actions::RANDOMIZE:
						iter_swap(next(laneMapper.begin(), r.action_src), next(laneMapper.begin(), static_cast<int>((POLY-1) * rng.flt())));
						break;
					case Actions::RESET:
						//just renumber lanMapper 0-Poly
					{
						for (int i = 0; i < laneMapper.size(); i++) {
							laneMapper[i] = i;
						}

					}

					break;
					case Actions::ENABLE_RULE:  //enable rule action_src
						if (r.action_src < rules.size()) {
							rules[r.action_src].run = true;
						}
						break;
					case Actions::DISABLE_RULE:  //disable rule action_src
						if (r.action_src < rules.size()) {
							rules[r.action_src].run = false;
							rules[r.action_src].event_counter = 0;
						}
						break;
					case Actions::TOGGLE_RULE:  //toggle rule action_src
						if (r.action_src < rules.size()) {
							rules[r.action_src].run = !rules[r.action_src].run;
							rules[r.action_src].event_counter = 0;
						}
						break;
					default:
						break;
					}

					r.event_counter = 0;
				}
			}
		}


		
		for (int i = 0; i < POLY; i++) {
			gates[getMappedLane(i)] = getLaneStepper(i)->process(clockProvider->gateData[static_cast<int>(lanes[i].clockType)]);
		}

		for (auto& r : rules) {
			if (r.run) {
				switch (r.event) {
				case Events::SEQUENCE_TRIGGER:
					//r.event_src -> lane stepper / gate index to read from
					r.event_counter += lanes[laneMapper[r.event_src]].triggered;
					break;
				case Events::SEQUENCE_END_OF_CYCLE:
					//r.event_counter += next(laneList.begin(), r.event_src)->eoc;
					r.event_counter += lanes[laneMapper[r.event_src]].eoc;
					break;
				case Events::CLOCK:
					//r.event_src -> clock provider to read from
					r.event_counter += clockProvider->tickData[r.event_src];
					break;
				default:
					break;
				}
			}

		}

	}


	virtual json toConfig() {
		json config;

		config["swing"] = clockProvider->swing;

		for (auto& l : lanes) {
			json lanej;
			lanej["type"] = l.type;
			lanej["clockType"] = l.clockType;

			config["lanes"].push_back(lanej);
		}

		
		for (int i = 0; i < POLY; i++) {
			config["stepSequencers"].push_back(stepSequencers[i]->toConfig());
		}

		for (int i = 0; i < POLY; i++) {
			config["euclidianDividers"].push_back(euclidianDividers[i]->toConfig());
		}

		for (auto& r : rules) {
			config["rules"].push_back(r.toConfig());
		}

		return config;
	}

	void loadConfig(const json cfg) {
		//clear();

		if(cfg.count("swing"))
			clockProvider->swing = cfg["swing"].get<float>();

		if (cfg.count("lanes")) {
			int l = 0;
			for (auto& lj : cfg["lanes"]) {
				lanes[l].type = static_cast<Types>(lj["type"].get<int>());
				lanes[l].clockType = static_cast<ClockProvider::ClockTypes>(lj["clockType"].get<int>());
				lanes[l].triggered = false;
				lanes[l].eoc = false;
				l++;
			}
		}
		

		if (cfg.count("stepSequencers")) {
			for (int i = 0; i < POLY; i++) {
				stepSequencers[i]->loadConfig(cfg["stepSequencers"][i]);
				//stepSequencers[i]->restart();
			}
		}
		

		if (cfg.count("euclidianDividers")) {
			for (int i = 0; i < POLY; i++) {
				euclidianDividers[i]->loadConfig(cfg["euclidianDividers"][i]);
				//euclidianDividers[i]->restart();
			}
		}
		

		if (cfg.count("rules")) {
			for (auto& rj : cfg["rules"]) {
				Rule<Events, Actions> newrule;
				newrule.loadConfig(rj);
				rules.push_back(newrule);
			}
		}
		

	}

};
























struct Life : Stepper {
	//Ran rng; hmmmm
	array<array<int, POLY>, POLY> grid;  
	array<array<int, POLY>, POLY> tmp;   
	
	Life() {
		length = POLY; // poly
		clear();
	}


	void Restart() {
		currentStep = 0;
	}

	int getCurrentStep() {
		return currentStep;
	}

	void clear() {
		length = POLY;   
		for (auto& a : grid) {
			for (auto& v : a) {
				v = 0;
			}
		}

		for (auto& a : tmp) {
			for (auto& v : a) {
				v = 0;
			}
		}

	}

	void jog(const int steps_) {
		currentStep = (currentStep + steps_) % length;
	}

	inline int wrap(const int i_) {
		return (i_ + POLY) % POLY; // poly
	}

	void channelConway() {

		for (auto& a : tmp) {
			for (auto& v : a) {
				v = 0;
			}
		}

		//count friends
		for (int i = 0; i < POLY; i++) {
			for (int j = 0; j < POLY; j++) {
				tmp[i][j] += grid[wrap(i - 1)][wrap(j - 1)];
				tmp[i][j] += grid[wrap(i + 1)][wrap(j - 1)];
				tmp[i][j] += grid[wrap(i + 1)][j];
				tmp[i][j] += grid[wrap(i + 1)][wrap(j + 1)];
				tmp[i][j] += grid[wrap(i - 1)][wrap(j + 1)];
				tmp[i][j] += grid[wrap(i - 1)][j];
				tmp[i][j] += grid[i][wrap(j - 1)];
				tmp[i][j] += grid[i][wrap(j + 1)];
			}
		}

		//Evaluate grid
		for (int i = 0; i < POLY; i++) {
			for (int j = 0; j < POLY; j++) {
				if (grid[i][j] == 0 && tmp[i][j] == 3) {
					grid[i][j] = 1;
				}
				else if (grid[i][j] == 1 && (tmp[i][j] < 2 || tmp[i][j] < 3)) {
					grid[i][j] = 0;
				}
			}
		}
	}

	int get(const int col_) {
		return grid[currentStep][col_];
	}

	bool inline step() {
		AdvanceStep();
		//channelConway();
		return endOfCycle;
	}
};

















































struct LifeSequencer {

	Trigger evolve_trigger;
	Trigger step_trigger;

	unique_ptr<ClockProvider> clock_provider;  //fine
	unique_ptr<Life> life;
	
	ClockProvider::ClockTypes evolve_clock_type;
	ClockProvider::ClockTypes step_clock_type;
	array<float, POLY> gates;									

	int getLife(const int x, const int y) {
		return life->grid[x][y];
	}

	void toggleLife(const int x, const int y) {
		life->grid[x][y] = !life->grid[x][y];
	}

	int getCurrentStep() {
		return life->getCurrentStep();
	}
	
	LifeSequencer() {
		clock_provider = make_unique<ClockProvider>();
		evolve_clock_type = ClockProvider::ClockTypes::NONE;
		step_clock_type = ClockProvider::ClockTypes::NONE;

		life = make_unique<Life>();

		for (auto& v : gates) {
			v = 0.0f;
		}

	}

	void clear() {
		;
	}


	void process(const float clkin) {
		clock_provider->process(bool(clkin));

		if (evolve_trigger.process(clock_provider->gateData[static_cast<int>(evolve_clock_type)])) {
			life->channelConway();
		}

		if (step_trigger.process(clock_provider->gateData[static_cast<int>(step_clock_type)])) {
			life->step();
		}

		for (int i = 0; i < POLY; i++) {
			gates[i] = life->get(i) && clock_provider->gateData[static_cast<int>(step_clock_type)];
		}

	}

	ClockProvider::ClockTypes getEvolveClockType() {
		return evolve_clock_type;
	}

	void setEvolveClockType(const ClockProvider::ClockTypes t_) {
		evolve_clock_type = t_;
	}

	ClockProvider::ClockTypes getStepClockType() {
		return step_clock_type;
	}

	void setStepClockType(const ClockProvider::ClockTypes t_) {
		step_clock_type = t_;
	}


	virtual json toConfig() {
		json config;
		config["evolve_clock_type"] = evolve_clock_type;
		config["step_clock_type"] = step_clock_type;

		for (int i = 0; i < POLY; i++) { 
			config["grid"].push_back(life->grid[i]);
		}

		return config;
	}

	void loadConfig(const json cfg) {
		//clear();

		if (cfg.count("evolve_clock_type"))
			evolve_clock_type = static_cast<ClockProvider::ClockTypes>(cfg["evolve_clock_type"].get<int>());

		if (cfg.count("step_clock_type"))
			step_clock_type = static_cast<ClockProvider::ClockTypes>(cfg["step_clock_type"].get<int>());

		if (cfg.count("grid")) {
			for (int i = 0; i < POLY; i++) { 
				for (int j = 0; j < POLY; j++) { 
					life->grid[i][j] = cfg["grid"][i][j].get<int>();
				}
			}
		}

	}

};



















//
//
//
//
//
//
//
//
//
//
//
//
//
////Not sure what to call these rules..
////
////These rules are evaluated on every
////measure type, same general idea as
////the step sequencer rules, but
////different scope
////This counts events on the gates
////and then can apply actions
////to measures and kits
////
////
////Hmmm how to track end of cycle events? hmmm
////Rethink note values? float perhaps, could use these for glide
////
//
//struct MetaRule {
//
//	enum Actions {
//		LOAD_KIT,		//Load kit indexed by action_src
//		SWAP_KIT,		//Swaps local kit element[action_src] with local kit element[action_dst]
//		ROTATE_KIT_FWD,	//Roates the local kit notes/samples indexed by action_src to action_dst by 1
//		ROTATE_KIT_BCK,	//Roates the local kit notes/samples indexed by action_src to action_dst by 1
//		TURING,			//On every action change local kit note[action_dst] action_dst loops from action_range_start-action_range_end
//						//random note from kit indexed by action_src
//		TRANSPOSE_NOTE,	//Transpose local kit note from action_range_start to action_range_end by action_src
//		WRITE_SCALE,	//Write atonal::Scale of action_src type action_dst rootN in range action_range_start..action_range_end, needs to wrap octaves
//		WRITE_CHORD,	//Write atonal::ChromaticSeventh of action_src type action_dst rootN in range action_range_start..action_range_end, needs to wrap octaves
//						//RiemannianTriads?
//		ENABLE_RULE,	//Enable rule indexed in action_src
//		DISABLE_RULE    //Disable rule indexed in action_src
//	};
//
//	bool run = false;
//	int gate_index = 0;
//	int event_counter = 0;
//	int action_threashold = 0;
//	Actions action = Actions::LOAD_KIT;
//	int action_src = 0;
//	int action_dst = 0;
//	int action_range_start = 0;
//	int action_range_end = 0;
//	int chain_rule = -1;
//
//};
//
//
///*
//
//	bool run = false;
//	Events event = Events::SEQUENCE_TRIGGER;
//	int event_src = 0;
//	int event_counter = 0;
//	int action_threashold = 0;
//	Actions action = Actions::SWAP;
//	int action_src = 0;
//	int action_dst = 0;
//	int chain_rule = -1;
//};
//*/
//
//
//
//
//
//
//













struct MeasureRunner {
//	enum Type {
//		EMPTY,
//		STEP,
//		MIDI,
//		LIFE
//	};

private:
	Measure::Type type = Measure::Type::EMPTY;

public:

	unique_ptr<EmptySequencer> empty = nullptr;
	unique_ptr<StepSequencer> step = nullptr;
	//unique_ptr<MidiProvider> midi = nullptr;
	unique_ptr<LifeSequencer> life = nullptr;

	shared_ptr<Measures> measures;
	bitBucket* bucket = nullptr;
	array<float, POLY> gates;
	int measure_idx = -1;

	MeasureRunner(shared_ptr<Measures> measures_) {
		measures = measures_;

		ifstream infile;
		infile.open("euclidean.bin", ios::in | ios::binary);

		infile.seekg(0, ios::end);
		const size_t num_elements = infile.tellg() / sizeof(bitBucket);
		infile.seekg(0, ios::beg);

		bucket = (bitBucket*)malloc(num_elements * sizeof(bitBucket));

		//Not sure how to quit out gracefully here
		infile.read(reinterpret_cast<char*>(bucket), num_elements * sizeof(bitBucket));

		infile.close();

		empty = make_unique<EmptySequencer>();
		step = make_unique<StepSequencer>(bucket);
		life = make_unique<LifeSequencer>();

		fill(gates.begin(), gates.end(), 0.0f);
	}

	Measure::Type &is() {
		return type;
	}

	int getMaxStepLength() {
		int m = 0;
		if (type == Measure::Type::STEP) {
			m = step->maxStepLength();
		}
		return m;
	}


	void setType(const Measure::Type t_) {
		switch (t_) {
		case Measure::Type::EMPTY:
			type = Measure::Type::EMPTY;
			break;
		case Measure::Type::STEP:
			type = Measure::Type::STEP;
		case Measure::Type::MIDI:
			//type = MIDI;
			break;
		case Measure::Type::LIFE:
			type = Measure::Type::LIFE;
			break;
		default:
			type = Measure::Type::EMPTY;
			break;
		}
	}

	void loadMeasure(const int measure_idx_) {

		measure_idx = measure_idx_;
		type = measures->getMeasureType(measure_idx);
		step->clear();


		switch (type) {
		case Measure::Type::EMPTY:
			break;
		case Measure::Type::STEP:
			step->loadConfig(measures->loadMeasure(measure_idx));
		case Measure::Type::MIDI:
			//type = MIDI;
			break;
		case Measure::Type::LIFE:
			life->loadConfig(measures->loadMeasure(measure_idx));
		default:
			break;
		}
	
	}

	void saveMeasure() {

		measures->setMeasureType(measure_idx, static_cast<Measure::Type>(type));
 
		switch (type) {
		case Measure::Type::EMPTY:
			break;
		case Measure::Type::STEP:
			measures->saveMeasure(measure_idx, step->toConfig());
		case Measure::Type::MIDI:
			//type = MIDI;
			break;
		case Measure::Type::LIFE:
			measures->saveMeasure(measure_idx, step->toConfig());
		default:
			break;
		}
	}

	void saveMeasure(const int mIdx_) {

		measures->setMeasureType(mIdx_, static_cast<Measure::Type>(type));

		switch (type) {
		case Measure::Type::EMPTY:
			break;
		case Measure::Type::STEP:
			measures->saveMeasure(mIdx_, step->toConfig());
		case Measure::Type::MIDI:
			//type = MIDI;
			break;
		case Measure::Type::LIFE:
			measures->saveMeasure(mIdx_, life->toConfig());
		default:
			break;
		}
	}

	//Redo with passed in gate array
	void process(const bool tick) {
		switch (type) {
		case Measure::Type::EMPTY:
			empty->process(tick);
			//copy out
			for (int i = 0; i < POLY; i++) {
				gates[i] = empty->gates[i];
			}
			break;
		case Measure::Type::STEP:
			step->process(tick);
			//copy out
			for (int i = 0; i < POLY; i++) {
				gates[i] = step->gates[i];
			}
			break;
		case Measure::Type::MIDI:
			//midi->process(channel, tick);
			break;
		case Measure::Type::LIFE:
			life->process(tick);
			//copy out
			for (int i = 0; i < POLY; i++) {
				gates[i] = life->gates[i];
			}
			break;
		default:
			break;
		}
	}

};
