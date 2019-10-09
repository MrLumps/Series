#pragma once

#include <memory>
#include <list>
#include <vector>
#include <array>
#include <list>
#include <string>
#include <sstream>
#include <fstream>
#include <bitset>
#include <algorithm>

#include "BaseObjects.h"
#include "imgui.h"
#include "Trigger.h"
#include "ModMatrix.h"

#include "Atonal.h"
#include "Stepper.h"

//#include "grids/base.h"
//#include "grids/clock.h"
//#include "grids/pattern_generator.h"
//
//uint16_t Random::rng_state_ = 0x21;
//#define F_CPU 20000000UL
//const uint8_t kUpdatePeriod = F_CPU / 32 / 8000;


using namespace std;

//Get sequencers to record user inputs
//Add a turing sequencer(s)


struct MasterClock {
private:
	float dclock = 0.0f;
	float phase = 0.0f;

public:

	void setFreq(const float dt_, const float hz_) {
		dclock = hz_ * dt_;
	}

	inline void Reset() {
		phase = 0.0f;
	}

	bool process() {
		bool tick = false;

		phase += dclock;
			   
		if (phase >= 1.0f) {
			phase -= 1.0f;
			tick = true;
		}

		return tick;
	}

};





//vector<std::string> simpleClockList{
//	"Whole",
//	"Half",
//	"Third",
//	"Quarter",
//	"Eighth",
//};
//
//struct SimpleClock : SignalUnit {
//	enum ClockTypes {
//		WHOLE,
//		HALF,
//		TRIPPLET,
//		QUARTER,
//		EIGTH
//	};
//
//
//private:
//	//While I would like to use the following
//	//const vector<int> pulseCounts = {96, 48, 32, 24, 12, 6};
//	//currently to drive grids with the master clock we need
//	const vector<int> pulseCounts = { 24, 12, 8, 6, 3 };
//	   
//	ClockTypes clocktype;
//	int pulseCount = 0;
//	int selectedClock = -1;
//
//public:
//	//0-1
//	//float swing = 0.0f;
//	float out = 0.0f;
//	
//	SimpleClock(const string name_, const float sampleRate_, Input* userParams_, shared_ptr<ModMatrix> matrix_, shared_ptr<NoteMatrix> noteMatrix_) {
//		sampleRate = sampleRate_;
//		name = name_;
//		userParams = userParams_;
//		matrix = matrix_;
//		noteMatrix = noteMatrix_;
//		clocktype = QUARTER;
//		Reset();
//
//		interfaceList = { {false, simpleClockList[static_cast<int>(clocktype)] + " out", 0.0f, 1.0f, &out } };
//
//		//matrix->registerProvider(name, 0.0f, 1.0f, &out);
//	}
//
//
//	inline void Reset() {
//		pulseCount = 0;
//		out = 0.0f;
//	}
//
//
//	void process(const bool tick_) {
//		pulseCount += static_cast<int>(tick_);
//		bool myTick = false;
//
//		if (pulseCount >= pulseCounts[static_cast<int>(clocktype)]) {
//			pulseCount = 0;
//			myTick = true;
//		}
//
//		out = (pulseCount < pulseCounts[static_cast<int>(clocktype)] / 2) ? 1.0f : 0.0f;
//	}
//
//	json toConfig() {
//		json config = ModuleBase::toConfig();
//
//		config["clocktype"] = static_cast<int>(clocktype);
//
//		return config;
//	}
//
//
//
//	void loadConfig(const json cfg) {
//
//		ModuleBase::loadConfig(cfg);
//
//		if (cfg.count("clocktype")) {
//			clocktype = static_cast<ClockTypes>(cfg["clocktype"].get<int>());
//		}
//
//		interfaceList[0].name = simpleClockList[static_cast<int>(clocktype)] + " out";
//
//	}
//
//
//
//	virtual void display() {
//		ImGui::PushID(this);
//		ImGui::Text(name.c_str()); //Shoud make this editable sometime
//
//		ImGui::PushID("clockSources");
//		if (ImGui::ListBox("Clock Out", &selectedClock, simpleClockList)) {
//			clocktype = static_cast<ClockTypes>(selectedClock);
//			interfaceList[0].name = simpleClockList[static_cast<int>(clocktype)] + " out";
//		}
//		ImGui::PopID();
//
//		ImGui::Separator();
//		ImGui::Text("Current Clock Output: ");
//		ImGui::SameLine();
//		ImGui::Text(simpleClockList[static_cast<int>(clocktype)].c_str());
//		ImGui::PopID();
//	}
//
//};




vector<std::string> clockSourceList{
	"Simple",
	"Pattern Map",
	"Somewhat Sequences"
};



struct Clock {

	string name;
	MasterClock master;
	//list< unique_ptr<SignalUnit> > clockSources;

	float dt;
	float bpm;

	Clock(const string name_, const float sampleRate_) {
		name = name_;

		dt = 1.0f / sampleRate_;
		bpm = 40;

	}
	

	//Needs to return something here
	bool process() {
		
		master.setFreq(dt, bpm * 0.4f); //const conversion factor from bpm to hz at 24 pulses per beat
		return master.process();

	}

	json toConfig() {
		json config;

		config["bpm"] = bpm;

		return config;
	}



	void loadConfig(const json cfg) {
		bpm = cfg["bpm"].get<float>();
	}


};





