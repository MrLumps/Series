#pragma once
#include <math.h>
#include <atomic>
#include <vector>
#include <condition_variable>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <utility>
#include <memory>
#include <thread>
#include <future>
#include <list>

#include "imgui.h"
#include "pffft.h"
#include "Filter.h"
#include "Effect.h"
#include "Envelope.h"
#include "Input.h"
#include "ModMatrix.h"
#include "Atonal.h"
#include "Stepper.h"
#include "Trigger.h"


using namespace std;





struct NConstant : SignalUnit {
private:
	array<Note, 5> notes = { 0, 0, 0, 0, 0 };
	Trigger trigger;
	float triggerIn = 0.0f;

public:

	NConstant(const string name_, const float sampleRate_, Input* userParams_, shared_ptr<ModMatrix> matrix_, shared_ptr<NoteMatrix> noteMatrix_) {
		sampleRate = sampleRate_;
		name = name_;
		userParams = userParams_;
		matrix = matrix_;
		noteMatrix = noteMatrix_;

		noteInterfaceList = {
			{false, name + " Out", &notes}
		};
		noteInterfaceList[0].pbSize = 5;

	}


	virtual void display() {
		ImGui::PushID(this);
		ImGui::Separator();
		ImGui::Text(name.c_str());

		for (int i = 0; i < notes.size(); i++) {
			ImGui::PushID(i);
			ImGui::DragInt(to_string(i).c_str(), &notes[i].n, 1.0f, -127, 127);
			ImGui::PopID();
		}

		ImGui::PopID();

	}

	json toConfig() {
		json config = ModuleBase::toConfig();

		for (auto& n : notes) {
			config["notes"].push_back(n.n);
		}
	
		return config;
	}



	void loadConfig(const json cfg) {

		ModuleBase::loadConfig(cfg);

		if (cfg.count("notes")) {
			for (int i = 0; i < cfg["notes"].size(); i++) {
				notes[i] = cfg["notes"][i].get<int>();
			}
		}


	}


};


struct NSandH : SignalUnit {
private:
	array<Note, 5> notes = { 0, 0, 0, 0, 0 };
	array<Note, 5> inputNotes = { 0, 0, 0, 0, 0 };
	Trigger trigger;
	float triggerIn = 0.0f;

public:

	NSandH(const string name_, const float sampleRate_, Input* userParams_, shared_ptr<ModMatrix> matrix_, shared_ptr<NoteMatrix> noteMatrix_) {
		sampleRate = sampleRate_;
		name = name_;
		userParams = userParams_;
		matrix = matrix_;
		noteMatrix = noteMatrix_;

		interfaceList = {
			{true, name + " Trigger", 0.0f, 1.0f, &triggerIn }
		};

		noteInterfaceList = {
			{false, name + " Out", &notes},
			{true, name + " In", &inputNotes}
		};

	}

	void process(const bool tick = false) {

		if (trigger.process(triggerIn)) {
			notes = inputNotes;
			noteInterfaceList[0].pbSize = noteInterfaceList[1].pbSize;
		}

	}

};


const vector<string> scaleTypes{
	"Major",
	"Natural Minor",
	"Harmonic Minor",
	"Melodic Minor",
	"Ionian",
	"Dorian",
	"Phrygian",
	"Lydian",
	"Mixolydian",
	"Aeolian",
	"Locian"
};


const vector<string> chordTypes{
	"Minor",
	"Major",
	"Dominant",
	"Augmented",
	"Diminished",
	"Sus2",
	"Sus4",
	"Major Sixth",
	"Minor Sixth",
	"Diatonic"
};

const vector<string> noteNames{
	"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

struct Chordulator : SignalUnit {

	array<Note, 5> notes = { 0, 0, 0, 0, 0 };
	array<Note, 5> inputNotes = { 0, 0, 0, 0, 0 };
	UnifiedChords chord;
	float modType = 0.0f;
	float modSize = 0.0f;
	float modScale = 0.0f;
	int inputCache = 0;

	bool showWindow = true;


	Chordulator(const string name_, const float sampleRate_, Input* userParams_, shared_ptr<ModMatrix> matrix_, shared_ptr<NoteMatrix> noteMatrix_) {
		name = name_;
		sampleRate = sampleRate_;
		userParams = userParams_;
		matrix = matrix_;
		noteMatrix = noteMatrix_;

		chord.root.n = 0;
		chord.type = UnifiedChords::Types::MINOR;
		chord.key.scale = ChromaticScale::ScaleTypes::MAJOR;
		chord.key.tonic.n = chord.root.n;

		inputCache = 0;
		modType = 0.0f;
		modSize = 0.0f;
		modScale = 0.0f;

		interfaceList = {
			{ true,	name + " Type", 0.0f, 1.0f, &modType },
			{ true,	name + " Size",  0.0f, 1.0f, &modSize },
			{ true,	name + " Scale",  0.0f, 1.0f, &modScale },
		};

		noteInterfaceList = {
			{false, name + " Out", &notes},
			{true, name + " In", &inputNotes}
		};

		updateNotes();

	}

	inline void updateNotes() {
		for (int i = 0; i < 5; i++) {
			notes[i].n = chord[i].n;
		}
	}

	void changeType(const UnifiedChords::Types t_) {
		chord.type = t_;
		//This seems goofy as heck, maybe need to rethink the modulation here
		modType = (1.0f / (UnifiedChords::Types::UNIFIED_TYPES - 1)) * t_;
	}


	virtual void process(const bool tick = false) {
		bool update = false;

		noteInterfaceList[0].pbSize = static_cast<int>(modSize * 5);

		const UnifiedChords::Types newtype = static_cast<UnifiedChords::Types>(static_cast<int>(modType * (UnifiedChords::Types::UNIFIED_TYPES - 1)));
		const ChromaticScale::ScaleTypes newscale = static_cast<ChromaticScale::ScaleTypes>(static_cast<int>(modScale * (ChromaticScale::ScaleTypes::SCALE_TYPES - 1)));

		if (chord.type != newtype) {
			chord.type = newtype;
			update = true;
		}

		if (chord.key.scale != newscale) {
			chord.key.scale = newscale;
			update = true;
		}

//		for (int k = 0; k < 12; k++) {
//			if (userParams->keys[k]) {
//				chord.root.relative(k);
//				chord.key.tonic.n = chord.root.n;
//				update = true;
//			}
//		}

		if (inputNotes[0].n != inputCache) {
			inputCache = inputNotes[0].n;
			chord.root.n = inputNotes[0].n;
			chord.key.tonic.n = chord.root.n;
			update = true;
		}

		if (update) {
			updateNotes();
		}

	}

	void display() {

		ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_FirstUseEver);
		ImGui::Begin(name.c_str(), &showWindow);
		ImGui::PushID(name.c_str());
		ImGui::Separator();


		ImGui::Text("Current Chord:"); ImGui::SameLine(); ImGui::Text(chordTypes[static_cast<int>(chord.type)].c_str());
		ImGui::Text("Current Root:"); ImGui::SameLine(); ImGui::Text(noteNames[chord.root.relative()].c_str());
		ImGui::Text("Current Size:"); ImGui::SameLine(); ImGui::Text("%d", noteInterfaceList[0].pbSize);
		ImGui::Text("Diatonic Scale:"); ImGui::SameLine(); ImGui::Text(scaleTypes[static_cast<int>(chord.key.scale)].c_str());

		ImGui::Separator();
		//Migrate to BaseObject
		int ifdx = 0;
		for (auto& ep : interfaceList) {
			ImGui::SliderFloat(ep.name.c_str(), ep.resP, ep.min, ep.max, "%.03f");
			mplusButton(ifdx);
			ifdx++;
		}

		ImGui::Separator();

		for (int i = 0; i < 5; i++) {
			if (i < noteInterfaceList[0].pbSize) {
				ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(notes[i].relative() / 12.0f, 0.8f, 0.8f));
			}
			else {
				ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(notes[i].relative() / 12.0f, 0.4f, 0.4f));
			}
			ImGui::SmallButton(noteNames[notes[i].relative()].c_str());
			ImGui::PopStyleColor();
			ImGui::SameLine();
		}

		ImGui::PopID();
		ImGui::End();
	}


	json toConfig() {
		json config = ModuleBase::toConfig();

		config["scale"] = static_cast<int>(chord.key.scale);
		config["tonic"] = chord.key.tonic.n;
		config["chord"] = chord.type;
		config["root"] = chord.root.n;
		config["modType"] = modType;
		config["modSize"] = modSize;

		return config;
	}


	void loadConfig(const json cfg) {

		ModuleBase::loadConfig(cfg);

		if (cfg.count("modType"))
			modType = cfg["modType"].get<float>();

		if (cfg.count("modSize"))
			modSize = cfg["modSize"].get<float>();

		if (cfg.count("scale"))
			chord.key.scale = static_cast<ChromaticScale::ScaleTypes>(cfg["scale"].get<int>());

		if (cfg.count("tonic"))
			chord.key.tonic.n = cfg["tonic"].get<int>();

		if (cfg.count("chord"))
			chord.type = static_cast<UnifiedChords::Types>(cfg["chord"].get<int>());

		if (cfg.count("root"))
			chord.root.n = cfg["root"].get<int>();


		updateNotes();
	}

};




struct Inversions : SignalUnit {
private:
	array<Note, 5> notes = { 0, 0, 0, 0, 0 };
	array<Note, 5> inputNotes = { 0, 0, 0, 0, 0 };
	float modInversion = 0.0f;

public:

	Inversions(const string name_, const float sampleRate_, Input* userParams_, shared_ptr<ModMatrix> matrix_, shared_ptr<NoteMatrix> noteMatrix_) {
		sampleRate = sampleRate_;
		name = name_;
		userParams = userParams_;
		matrix = matrix_;
		noteMatrix = noteMatrix_;

		interfaceList = {
			{true, name + " Select", -1.0f, 1.0f, &modInversion },

		};

		noteInterfaceList = {
			{false, name + " Out", &notes},
			{true, name + " In", &inputNotes}
		};

	}

	//Meh ugly
	//1st - note 0 + 12
	//2nd - note 0,1 + 12... all notes -12
	//3rd - note 0,1,2 + 12 ... all notes -12
	void process(const bool tick = false) {

		noteInterfaceList[0].pbSize = noteInterfaceList[1].pbSize;

		if (modInversion < 0.0f) {
			if (modInversion < -0.2f) {
				notes[0] = inputNotes[0];
				notes[1] = inputNotes[1];
				notes[2] = inputNotes[2];
				notes[3] = inputNotes[3];
				notes[4].n = inputNotes[4].n - 12;
			}
			else if (modInversion < -0.4f) {
				notes[0] = inputNotes[0];
				notes[1] = inputNotes[1];
				notes[2] = inputNotes[2];
				notes[3].n = inputNotes[3].n - 12;
				notes[4].n = inputNotes[4].n - 12;
			}
			else if (modInversion < -0.6f) {
				notes[0] = inputNotes[0];
				notes[1] = inputNotes[1];
				notes[2].n = inputNotes[2].n-12;
				notes[3].n = inputNotes[3].n-12;
				notes[4].n = inputNotes[4].n-12;
			}
			else if (modInversion < -0.8f) {
				notes[0] = inputNotes[0];
				notes[2].n = inputNotes[2].n - 12;
				notes[1].n = inputNotes[1].n - 12;
				notes[3].n = inputNotes[3].n - 12;
				notes[4].n = inputNotes[4].n - 12;
			}
			else {
				notes[0] = inputNotes[0];
				notes[1] = inputNotes[1];
				notes[2] = inputNotes[2];
				notes[3] = inputNotes[3];
				notes[4] = inputNotes[4];
			}
		}
		else {
			if (modInversion < 0.2f) {
				notes[0] = inputNotes[0];
				notes[1] = inputNotes[1];
				notes[2] = inputNotes[2];
				notes[3] = inputNotes[3];
				notes[4] = inputNotes[4];
			} else if (modInversion < 0.4f) {
				notes[0].n = inputNotes[0].n + 12;
				notes[1] = inputNotes[1];
				notes[2] = inputNotes[2];
				notes[3] = inputNotes[3];
				notes[4] = inputNotes[4];
			} else if (modInversion < 0.6f) {
				notes[0].n = inputNotes[0].n + 12;
				notes[1].n = inputNotes[1].n + 12;
				notes[2] = inputNotes[2];
				notes[3] = inputNotes[3];
				notes[4] = inputNotes[4];
			} else if (modInversion < 0.8f) {
				notes[0].n = inputNotes[0].n + 12;
				notes[1].n = inputNotes[1].n + 12;
				notes[2].n = inputNotes[2].n + 12;
				notes[3] = inputNotes[3];
				notes[4] = inputNotes[4];
			} else {
				notes[0].n = inputNotes[0].n + 12;
				notes[1].n = inputNotes[1].n + 12;
				notes[2].n = inputNotes[2].n + 12;
				notes[3].n = inputNotes[3].n + 12;
				notes[4] = inputNotes[4];
			}
		}

	}

};





struct Voicings : SignalUnit {
private:
	array<Note, 5> notes = { 0, 0, 0, 0, 0 };
	array<Note, 5> inputNotes = { 0, 0, 0, 0, 0 };
	float modVoicing = 0.0f;

public:

	Voicings(const string name_, const float sampleRate_, Input* userParams_, shared_ptr<ModMatrix> matrix_, shared_ptr<NoteMatrix> noteMatrix_) {
		sampleRate = sampleRate_;
		name = name_;
		userParams = userParams_;
		matrix = matrix_;
		noteMatrix = noteMatrix_;

		interfaceList = {
			{true, name + " Select", -1.0f, 1.0f, &modVoicing },

		};

		noteInterfaceList = {
			{false, name + " Out", &notes},
			{true, name + " In", &inputNotes}
		};

	}

	//Also ugly
	void process(const bool tick = false) {

		noteInterfaceList[0].pbSize = noteInterfaceList[1].pbSize;

		if (modVoicing < 0.0f) {
			if (modVoicing < -0.2f) {
				notes[0] = inputNotes[0];
				notes[1] = inputNotes[1];
				notes[2] = inputNotes[2];
				notes[3] = inputNotes[3];
				notes[4] = inputNotes[4];
			}
			else if (modVoicing < -0.4f) {
				notes[0].n = inputNotes[0].n - 12;
				notes[1]   = inputNotes[1];
				notes[2].n = inputNotes[2].n - 12;
				notes[3].n = inputNotes[3].n - 12;
				notes[4].n = inputNotes[4].n - 12;
			}
			else if (modVoicing < -0.6f) {
				notes[0].n = inputNotes[0].n - 12;
				notes[1].n = inputNotes[1].n - 12;
				notes[2]   = inputNotes[2];
				notes[3].n = inputNotes[3].n - 12;
				notes[4].n = inputNotes[4].n - 12;
			}
			else if (modVoicing < -0.8f) {
				notes[0].n = inputNotes[0].n - 12;
				notes[1].n = inputNotes[1].n - 12;
				notes[2].n = inputNotes[2].n - 12;
				notes[3]   = inputNotes[3];
				notes[4].n = inputNotes[4].n - 12;
			}
			else {
				notes[0].n = inputNotes[0].n - 12;
				notes[1]   = inputNotes[1];
				notes[2]   = inputNotes[2];
				notes[3]   = inputNotes[3];
				notes[4].n = inputNotes[4].n - 12;
			}

		} 
		else {
			if (modVoicing < 0.2f) {
				notes[0] = inputNotes[0];
				notes[1] = inputNotes[1];
				notes[2] = inputNotes[2];
				notes[3] = inputNotes[3];
				notes[4] = inputNotes[4];
			}
			else if (modVoicing < 0.4f) {
				notes[0] = inputNotes[0];
				notes[1].n = inputNotes[1].n + 12;
				notes[2] = inputNotes[2];
				notes[3] = inputNotes[3];
				notes[4] = inputNotes[4];
			}
			else if (modVoicing < 0.6f) {
				notes[0] = inputNotes[0];
				notes[1] = inputNotes[1];
				notes[2].n = inputNotes[2].n + 12;
				notes[3] = inputNotes[3];
				notes[4] = inputNotes[4];
			}
			else if (modVoicing < 0.8f) {
				notes[0] = inputNotes[0];
				notes[1] = inputNotes[1];
				notes[2] = inputNotes[2];
				notes[3].n = inputNotes[3].n + 12;
				notes[4] = inputNotes[4];
			}
			else {
				notes[0] = inputNotes[0];
				notes[1].n = inputNotes[1].n + 12;
				notes[2].n = inputNotes[2].n + 12;
				notes[3].n = inputNotes[3].n + 12;
				notes[4] = inputNotes[4];
			}
		}

	}

};

struct ArpStepper : Stepper {
public:
	int getCurrentStep() {
		return currentStep;
	}

	void Step() {
		AdvanceStep();
	}

};



struct Arpeggiator : SignalUnit {
private:
	array<Note, 5> notes = { 0, 0, 0, 0, 0 };
	array<Note, 5> inputNotes = { 0, 0, 0, 0, 0 };
	Trigger trigger;
	ArpStepper stepper;
	float triggerIn = 0.0f;
	float modeIn = 0.0f;

public:

	Arpeggiator(const string name_, const float sampleRate_, Input* userParams_, shared_ptr<ModMatrix> matrix_, shared_ptr<NoteMatrix> noteMatrix_) {
		sampleRate = sampleRate_;
		name = name_;
		userParams = userParams_;
		matrix = matrix_;
		noteMatrix = noteMatrix_;

		interfaceList = {
			{true, name + " Trigger", 0.0f, 1.0f, &triggerIn },
			{true, name + " Mode", 0.0f, 1.0f, &modeIn },
		};

		noteInterfaceList = {
			{false, name + " Out", &notes},
			{true, name + " In", &inputNotes}
		};

		noteInterfaceList[0].pbSize = 1;
		stepper.length = 1;
		stepper.SetMode(Stepper::Modes::FORWARD_MODE);
	}

	void process(const bool tick = false) {

		if (modeIn < 0.25f) {
			stepper.SetMode(Stepper::Modes::FORWARD_MODE);
		}
		else if (modeIn < 0.5f) {
			stepper.SetMode(Stepper::Modes::REVERSE_MODE);
		}
		else if (modeIn < 0.75f) {
			stepper.SetMode(Stepper::Modes::BOUNCE_MODE);
		}
		else {
			stepper.SetMode(Stepper::Modes::RANDOM_MODE);
		}

		stepper.length = noteInterfaceList[1].pbSize;

		if (trigger.process(triggerIn)) {
			stepper.Step();
			notes[0] = inputNotes[stepper.getCurrentStep()];
		}

		

	}

};










struct ManyToOneNSwitch : SignalUnit {
private:
	array<Note, 5> notes = { 0, 0, 0, 0, 0 };
	array<array<Note, 5>, 8> inputNotes{{
		{0,0,0,0,0},
		{0,0,0,0,0},
		{0,0,0,0,0},
		{0,0,0,0,0},
		{0,0,0,0,0},
		{0,0,0,0,0},
		{0,0,0,0,0},
		{0,0,0,0,0}
	}};
	array<int, 8> stepsPerInput = { 1, 1, 1, 1, 1, 1, 1, 1 };
	SwitchStepper stepper;
	Trigger trigger;
	float triggerIn = 0.0f;
	float ballance = 0.0f;
	float divisor = 1.0f;
	float mode = 0.0f;
	int triggerCounter = 0;

public:

	ManyToOneNSwitch(const string name_, const float sampleRate_, Input* userParams_, shared_ptr<ModMatrix> matrix_, shared_ptr<NoteMatrix> noteMatrix_) {
		sampleRate = sampleRate_;
		name = name_;
		userParams = userParams_;
		matrix = matrix_;
		noteMatrix = noteMatrix_;

		interfaceList = {
			{true, name + " Trigger", 0.0f, 1.0f, &triggerIn },
			{true, name + " Div", 1.0f, 256.0f, &divisor },
			{true, name + " Ballance", -1.0f, 1.0f, &ballance },
		};

		noteInterfaceList.reserve(8 + 1);
		noteInterfaceList = {
			{false, name + " Out", &notes},
			{true, name + " In1", &inputNotes[0] },
			{true, name + " In2", &inputNotes[1] },
			{true, name + " In3", &inputNotes[2] },
			{true, name + " In4", &inputNotes[3] },
			{true, name + " In5", &inputNotes[4] },
			{true, name + " In6", &inputNotes[5] },
			{true, name + " In7", &inputNotes[6] },
			{true, name + " In8", &inputNotes[7] }
		};

		stepper.length = 0;

	}

	virtual void display() {
		ImGui::PushID(this);
		ImGui::Separator();
		ImGui::Text(name.c_str());
		
		ImGui::Text("Number of Inputs: %d", stepper.length);
		ImGui::Text("Current Input: %d", stepper.getCurrentStep());
		if (ImGui::Button("Add Input")) {
			const int i = stepper.length;
			if (i < 8) {
				registerNoteEndPoint(i+1);
				stepper.length += 1;
			}
			
		}

		ImGui::NewLine();

		//for (int i = 0; i < stepsPerInput.size(); i++) {
		//	ImGui::SameLine();
		//	ImGui::Text("%d", stepsPerInput[i]);
		//}

		//for (int i = 1; i < interfaceList.size(); i++) {
		//	ImGui::DragFloat(interfaceList[i].name.c_str(), interfaceList[i].resP, 0.001f, interfaceList[i].min, interfaceList[i].max, "%.03f");
		//	mplusButton(i);
		//}
		ImGui::PopID();

	}


	void process(const bool tick = false) {

		if (mode < 0.25f) {
			stepper.SetMode(Stepper::Modes::FORWARD_MODE);
		} else if (mode < 0.5f) {
			stepper.SetMode(Stepper::Modes::REVERSE_MODE);
		} else if (mode < 0.75f) {
			stepper.SetMode(Stepper::Modes::BOUNCE_MODE);
		} else {
			stepper.SetMode(Stepper::Modes::RANDOM_MODE);
		}

		if (trigger.process(triggerIn)) {
			triggerCounter++;
			if (triggerCounter > stepsPerInput[stepper.getCurrentStep()]) {
				stepper.Step();
				triggerCounter = 0;
			}
		}

		if (noteInterfaceList.size() > 1) {
			(*noteInterfaceList[0].resP) = (*noteInterfaceList[stepper.getCurrentStep() + 1].resP);
			noteInterfaceList[0].pbSize = noteInterfaceList[stepper.getCurrentStep() + 1].pbSize;
		}

	}




	/*
		array<Note, 5> notes = { 0, 0, 0, 0, 0 };
	array<array<Note, 5>, 8> inputNotes;
	array<int, 8> stepsPerInput = { 1, 1, 1, 1, 1, 1, 1, 1 };
	*/




	json toConfig() {
		json config = ModuleBase::toConfig();

		config["length"] = stepper.length;

		for (int i = 0; i < 8; i++) {
			config["steps"].push_back(stepsPerInput[i]);
		}

		return config;
	}


	void loadConfig(const json cfg) {

		ModuleBase::loadConfig(cfg);

		if (cfg.count("length"))
			stepper.length = cfg["length"].get<int>();

		if (cfg.count("steps")) {
			for (int i = 0; i < 8; i++) {
				stepsPerInput[i] = cfg["steps"][i].get<int>();
			}
		}


	}

	   	  
};




struct NoteScope : SignalUnit {
private:
	array<Note, 5> notes = { 0, 0, 0, 0, 0 };

public:

	NoteScope(const string name_, const float sampleRate_, Input* userParams_, shared_ptr<ModMatrix> matrix_, shared_ptr<NoteMatrix> noteMatrix_) {
		sampleRate = sampleRate_;
		name = name_;
		userParams = userParams_;
		matrix = matrix_;
		noteMatrix = noteMatrix_;

		noteInterfaceList = {
			{true, name + " In", &notes},
		};

	}

	void display() {
		ImGui::PushID(this);
		ImGui::Separator();
		ImGui::Text(name.c_str());

		ImGui::Text("PB Size: %d", noteInterfaceList[0].pbSize);
		ImGui::Text("Current Notes:");
		for (auto& n : notes) {
			ImGui::Text(noteNames[n.relative()].c_str());
			ImGui::SameLine();
		}
	
		ImGui::NewLine();

		for (auto& n : notes) {
			ImGui::Text("%d", n.n);
			ImGui::SameLine();
		}

		ImGui::PopID();

	}


};















////uses input notes as scale root
////has 0-7
////2 trigger
//
//struct Scaliator : SignalUnit {
//private:
//	array<Note, 5> notes = { 0, 0, 0, 0, 0 };
//	array<Note, 5> inputNotes = { 0, 0, 0, 0, 0 };
//	Trigger trigger;
//	ArpStepper stepper;
//	float triggerIn = 0.0f;
//	float modeIn = 0.0f;
//
//public:
//
//	Scaliator(const string name_, const float sampleRate_, Input* userParams_, shared_ptr<ModMatrix> matrix_, shared_ptr<NoteMatrix> noteMatrix_) {
//		sampleRate = sampleRate_;
//		name = name_;
//		userParams = userParams_;
//		matrix = matrix_;
//		noteMatrix = noteMatrix_;
//
//		interfaceList = {
//			{true, name + " Trigger", 0.0f, 1.0f, &triggerIn },
//			{true, name + " Mode", 0.0f, 1.0f, &modeIn },
//		};
//
//		noteInterfaceList = {
//			{false, name + " Out", &notes},
//			{true, name + " In", &inputNotes}
//		};
//
//		noteInterfaceList[0].pbSize = 1;
//		stepper.length = 1;
//		stepper.SetMode(Stepper::Modes::FORWARD_MODE);
//	}
//
//	void process(const bool tick = false) {
//
//		if (modeIn < 0.25f) {
//			stepper.SetMode(Stepper::Modes::FORWARD_MODE);
//		}
//		else if (modeIn < 0.5f) {
//			stepper.SetMode(Stepper::Modes::REVERSE_MODE);
//		}
//		else if (modeIn < 0.75f) {
//			stepper.SetMode(Stepper::Modes::BOUNCE_MODE);
//		}
//		else {
//			stepper.SetMode(Stepper::Modes::RANDOM_MODE);
//		}
//
//		stepper.length = noteInterfaceList[1].pbSize;
//
//		if (trigger.process(triggerIn)) {
//			stepper.Step();
//			notes[0] = inputNotes[stepper.getCurrentStep()];
//		}
//
//
//
//	}
//
//};
//





//Random Note Sequencer
//This has the effect of randomly picking notes from the input set 0-4
//With a chance of changing that note per step
struct Turing : SignalUnit {
private:
	array<int, 16> state = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	array<Note, 5> notes = { 0, 0, 0, 0, 0 };
	array<Note, 5> inputNotes = { 0, 0, 0, 0, 0 };
	Trigger trigger;
	ArpStepper stepper;
	Ran rng;
	float triggerIn = 0.0f;
	float modeIn = 0.0f;
	float modLength = 1.0f;
	float modChance = 0.0f;

public:

	Turing(const string name_, const float sampleRate_, Input* userParams_, shared_ptr<ModMatrix> matrix_, shared_ptr<NoteMatrix> noteMatrix_) {
		sampleRate = sampleRate_;
		name = name_;
		userParams = userParams_;
		matrix = matrix_;
		noteMatrix = noteMatrix_;

		interfaceList = {
			{true, name + " Trigger", 0.0f, 1.0f, &triggerIn },
			{true, name + " Mode", 0.0f, 1.0f, &modeIn },
			{true, name + " Length", 0.0f, 1.0f, &modLength },
			{true, name + " Chance", 0.0f, 1.0f, &modChance }
		};

		noteInterfaceList = {
			{false, name + " Out", &notes},
			{true, name + " In", &inputNotes}
		};

		noteInterfaceList[0].pbSize = 1;
		stepper.length = 1;
		stepper.SetMode(Stepper::Modes::FORWARD_MODE);

		for (auto& s : state) {
			s = rng.flt() * 4;
		}

	}

	void process(const bool tick = false) {

		stepper.SetMode(Stepper::Modes::NUM_MODES * modeIn);
		stepper.length = modLength * 16;

		if (trigger.process(triggerIn)) {
			stepper.Step();

			const auto current = stepper.getCurrentStep();

			//Coin flip for state update
			if (rng.flt() < modChance) {
				state[current] = rng.flt() * (noteInterfaceList[1].pbSize - 1);
			}

			notes[0] = inputNotes[state[current]];
		}

	}


	json toConfig() {
		json config = ModuleBase::toConfig();

		config["length"] = stepper.length;

		for (int i = 0; i < 16; i++) {
			config["state"].push_back(state[i]);
		}

		return config;
	}


	void loadConfig(const json cfg) {

		ModuleBase::loadConfig(cfg);

		if (cfg.count("length"))
			stepper.length = cfg["length"].get<int>();

		if (cfg.count("state")) {
			for (int i = 0; i < 8; i++) {
				state[i] = cfg["state"][i].get<int>();
			}
		}


	}










};






































vector<string> noteUtilitiesList{
	"Constant",
	"Sample and Hold",
	"Chordulator",
	"Inversions",
	"Voicings",
	"Arpeggiator",
	"N:1 Switch",
	"Turing",
	"Note Scope"
};




struct NoteUtilities : SignalUnit {

	list< shared_ptr<SignalUnit> > noteUtil;
	int selectedUnit = -1;

	array<char, 32> namebuffer;

	bool setUserParams = false;
	bool showWindow = true;

	unordered_map<string, shared_ptr<ModuleBase>> activeModules;

	NoteUtilities(const string name_, const float sampleRate_, Input* userParams_, shared_ptr<ModMatrix> matrix_, shared_ptr<NoteMatrix> noteMatrix_) {
		sampleRate = sampleRate_;
		name = name_;
		userParams = userParams_;
		matrix = matrix_;
		noteMatrix = noteMatrix_;

		fill(namebuffer.begin(), namebuffer.end(), 0);
	}

	void process(const bool tick = false) {
		for (auto& clku : noteUtil) {
			clku->process();
		}
	}

	void display() {
		ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_FirstUseEver);
		ImGui::Begin(name.c_str(), &showWindow);
		ImGui::PushID(name.c_str());
		ImGui::Separator();

		ImGui::Text("Note Utilities");
		if (ImGui::ListBox("", &selectedUnit, noteUtilitiesList)) {
			std::copy(noteUtilitiesList[selectedUnit].begin(), noteUtilitiesList[selectedUnit].end(), namebuffer.data());
			ImGui::OpenPopup("AddUnit");
		}


		if (ImGui::BeginPopup("AddUnit"))
		{
			if (ImGui::InputText("Note Utility Name", namebuffer.data(), namebuffer.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
				switch (selectedUnit) {
				case 0:
					noteUtil.push_back(make_unique<NConstant>(namebuffer.data(), sampleRate, userParams, matrix, noteMatrix));
					noteUtil.back()->registerNoteEndPoint(0);
					activeModules[namebuffer.data()] = dynamic_pointer_cast<ModuleBase, SignalUnit>(noteUtil.back());
					break;
				case 1:
					noteUtil.push_back(make_unique<NSandH>(namebuffer.data(), sampleRate, userParams, matrix, noteMatrix));
					noteUtil.back()->registerEndPoint(0);
					noteUtil.back()->registerNoteEndPoint(0);
					noteUtil.back()->registerNoteEndPoint(1);
					activeModules[namebuffer.data()] = dynamic_pointer_cast<ModuleBase, SignalUnit>(noteUtil.back());
					break;
				case 2:
					noteUtil.push_back(make_unique<Chordulator>(namebuffer.data(), sampleRate, userParams, matrix, noteMatrix));
					noteUtil.back()->registerNoteEndPoint(0);
					noteUtil.back()->registerNoteEndPoint(1);
					activeModules[namebuffer.data()] = dynamic_pointer_cast<ModuleBase, SignalUnit>(noteUtil.back());
					break;
				case 3:
					noteUtil.push_back(make_unique<Inversions>(namebuffer.data(), sampleRate, userParams, matrix, noteMatrix));
					noteUtil.back()->registerNoteEndPoint(0);
					noteUtil.back()->registerNoteEndPoint(1);
					activeModules[namebuffer.data()] = dynamic_pointer_cast<ModuleBase, SignalUnit>(noteUtil.back());
					break;
				case 4:
					noteUtil.push_back(make_unique<Voicings>(namebuffer.data(), sampleRate, userParams, matrix, noteMatrix));
					noteUtil.back()->registerNoteEndPoint(0);
					noteUtil.back()->registerNoteEndPoint(1);
					activeModules[namebuffer.data()] = dynamic_pointer_cast<ModuleBase, SignalUnit>(noteUtil.back());
					break;
				case 5:
					noteUtil.push_back(make_unique<Arpeggiator>(namebuffer.data(), sampleRate, userParams, matrix, noteMatrix));
					noteUtil.back()->registerEndPoint(0);
					noteUtil.back()->registerNoteEndPoint(0);
					noteUtil.back()->registerNoteEndPoint(1);
					activeModules[namebuffer.data()] = dynamic_pointer_cast<ModuleBase, SignalUnit>(noteUtil.back());
					break;
				case 6:
					noteUtil.push_back(make_unique<ManyToOneNSwitch>(namebuffer.data(), sampleRate, userParams, matrix, noteMatrix));
					noteUtil.back()->registerEndPoint(0);
					noteUtil.back()->registerNoteEndPoint(0);
					activeModules[namebuffer.data()] = dynamic_pointer_cast<ModuleBase, SignalUnit>(noteUtil.back());
					break;
				case 7:
					noteUtil.push_back(make_unique<Turing>(namebuffer.data(), sampleRate, userParams, matrix, noteMatrix));
					noteUtil.back()->registerEndPoint(0);
					noteUtil.back()->registerNoteEndPoint(0);
					noteUtil.back()->registerNoteEndPoint(1);
					activeModules[namebuffer.data()] = dynamic_pointer_cast<ModuleBase, SignalUnit>(noteUtil.back());
					break;
				case 8:
					noteUtil.push_back(make_unique<NoteScope>(namebuffer.data(), sampleRate, userParams, matrix, noteMatrix));
					noteUtil.back()->registerNoteEndPoint(0);
					activeModules[namebuffer.data()] = dynamic_pointer_cast<ModuleBase, SignalUnit>(noteUtil.back());
					break;

				default:
					break;
				}
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::PushID("noteUtilsList");
		for (auto& clku : noteUtil) {
			ImGui::Separator();
			clku->display();
		}
		ImGui::PopID();



		ImGui::PopID();
		ImGui::End();
	}

	json toConfig() {
		json config = ModuleBase::toConfig();

		config["setUserParams"] = setUserParams;

		for (auto& mods : noteUtil) {
			config["noteUtil"].push_back(mods->toConfig());
		}
		return config;
	}



	void loadConfig(const json cfg) {

		ModuleBase::loadConfig(cfg);

		if (cfg.count("noteUtil")) {
			for (auto& clkuj : cfg["noteUtil"]) {
				if (clkuj.count("type")) {
					if (clkuj["type"].get<string>().compare("struct NConstant") == 0) {
						noteUtil.push_back(make_shared<NConstant>(clkuj["name"].get<string>(), sampleRate, userParams, matrix, noteMatrix));
						noteUtil.back()->loadConfig(clkuj);
						activeModules[clkuj["name"].get<string>()] = dynamic_pointer_cast<ModuleBase, SignalUnit>(noteUtil.back());
					}
					if (clkuj["type"].get<string>().compare("struct Inversions") == 0) {
						noteUtil.push_back(make_shared<Inversions>(clkuj["name"].get<string>(), sampleRate, userParams, matrix, noteMatrix));
						noteUtil.back()->loadConfig(clkuj);
						activeModules[clkuj["name"].get<string>()] = dynamic_pointer_cast<ModuleBase, SignalUnit>(noteUtil.back());
					}
					if (clkuj["type"].get<string>().compare("struct Voicings") == 0) {
						noteUtil.push_back(make_shared<Voicings>(clkuj["name"].get<string>(), sampleRate, userParams, matrix, noteMatrix));
						noteUtil.back()->loadConfig(clkuj);
						activeModules[clkuj["name"].get<string>()] = dynamic_pointer_cast<ModuleBase, SignalUnit>(noteUtil.back());
					}
					if (clkuj["type"].get<string>().compare("struct NSandH") == 0) {
						noteUtil.push_back(make_shared<NSandH>(clkuj["name"].get<string>(), sampleRate, userParams, matrix, noteMatrix));
						noteUtil.back()->loadConfig(clkuj);
						activeModules[clkuj["name"].get<string>()] = dynamic_pointer_cast<ModuleBase, SignalUnit>(noteUtil.back());
					}
					if (clkuj["type"].get<string>().compare("struct Arpeggiator") == 0) {
						noteUtil.push_back(make_shared<Arpeggiator>(clkuj["name"].get<string>(), sampleRate, userParams, matrix, noteMatrix));
						noteUtil.back()->loadConfig(clkuj);
						activeModules[clkuj["name"].get<string>()] = dynamic_pointer_cast<ModuleBase, SignalUnit>(noteUtil.back());
					}
					if (clkuj["type"].get<string>().compare("struct Chordulator") == 0) {
						noteUtil.push_back(make_shared<Chordulator>(clkuj["name"].get<string>(), sampleRate, userParams, matrix, noteMatrix));
						noteUtil.back()->loadConfig(clkuj);
						activeModules[clkuj["name"].get<string>()] = dynamic_pointer_cast<ModuleBase, SignalUnit>(noteUtil.back());
					}
					if (clkuj["type"].get<string>().compare("struct ManyToOneNSwitch") == 0) {
						noteUtil.push_back(make_shared<ManyToOneNSwitch>(clkuj["name"].get<string>(), sampleRate, userParams, matrix, noteMatrix));
						noteUtil.back()->loadConfig(clkuj);
						activeModules[clkuj["name"].get<string>()] = dynamic_pointer_cast<ModuleBase, SignalUnit>(noteUtil.back());
					}
					if (clkuj["type"].get<string>().compare("struct Turing") == 0) {
						noteUtil.push_back(make_shared<Turing>(clkuj["name"].get<string>(), sampleRate, userParams, matrix, noteMatrix));
						noteUtil.back()->loadConfig(clkuj);
						activeModules[clkuj["name"].get<string>()] = dynamic_pointer_cast<ModuleBase, SignalUnit>(noteUtil.back());
					}
					if (clkuj["type"].get<string>().compare("struct NoteScope") == 0) {
						noteUtil.push_back(make_shared<NoteScope>(clkuj["name"].get<string>(), sampleRate, userParams, matrix, noteMatrix));
						noteUtil.back()->loadConfig(clkuj);
						activeModules[clkuj["name"].get<string>()] = dynamic_pointer_cast<ModuleBase, SignalUnit>(noteUtil.back());
					}

				}
			}
		}
	}




};


