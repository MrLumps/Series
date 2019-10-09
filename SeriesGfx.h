#pragma once

//
// Enable the gui to select the current channel to cut down on clutter
//

#include "Engine/Series.h"
#include "Engine/Common.h"

#include "GL\glew.h"
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imguiCustom.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"


#include <iostream>
#include <sstream>


#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glfw3.lib")
#pragma comment(lib, "imgui.lib")

using namespace std;


//Hacky - >>>> Move
struct VisEnvelope {
	enum State {
		IDLE,
		ATTACK,
		DECAY,
		SUSTAIN,
		RELEASE
	};

	Trigger inputTrigger;
	float dt = 0.0f;
	float attackTime = 0.0f;
	float decayTime = 0.0f;
	float sustainLevel = 0.0f;
	float releaseTime = 0.0f;
	float shape = 0.0f;
	float phase = 0.0f;
	State state = IDLE;


	void trigger() {
		state = ATTACK;
	}

	float process(const float in_) {
		if (inputTrigger.process(in_)) {
			trigger();
		}

		switch (state) {

		case IDLE:
			return 0.0f;
			break;

		case ATTACK:
			phase += 1.0f / ((attackTime / 1000.0f) / dt);

			if (phase >= 1.0f) {
				phase = 1.0f;
				state = DECAY;
			}

			//return exp(shape * phase) - 1.0f / exp(shape) - 1.0f;
			return phase;
			break;

		case DECAY:
			if (phase <= sustainLevel || decayTime <= 0.0f) {
				state = SUSTAIN;
			}
			else {
				phase -= (1.0f - sustainLevel) / ((decayTime / 1000.0f) / dt);
			}

			return phase;
			break;

		case SUSTAIN:
			if (in_ < sustainLevel) {
				state = RELEASE;
			}
			phase = sustainLevel;

			return phase;
			break;

		case RELEASE:
			phase -= sustainLevel / ((releaseTime / 100.0f) / dt);

			if (phase <= 0.0f) {
				phase = 0.0f;
				state = IDLE;
			}

			return phase;
			break;

		}

		return 0.0f;
	}

};



//Seperate gfx context for Series
//Ugly but quick
struct SeriesGfx {

	float rescale(const float x_, const float xMin_, const float xMax_, const float yMin_, const float yMax_) {
		return yMin_ + (x_ - xMin_) / (xMax_ - xMin_) * (yMax_ - yMin_);
	}

	const int win_w = 1708;
	const int win_h = 960;
	bool show_save_window = true;

	char audioSaveFile[128] = "series.json";
	char audioLoadFile[128] = "series.json";

	//OpenGL variables
	GLFWwindow* window = nullptr;
	Series* audio = nullptr;



	bool showClock = true;
	bool showMixer = true;
	bool showModMatrix = true;


	vector<string> clockSourceNames = { "None",  "Whole", "Half", "Tripplet", "Quarter", "Eighth", "Double", "Tripple", "Quadruple" };
	ClockProvider::ClockTypes currentClockSource = ClockProvider::ClockTypes::NONE;

	vector<string> stepSequenceTypeNames = { "None", "Step", "Euclidian" };
	StepSequencer::Types currentSequenceType = StepSequencer::Types::NONE;


	float scopeScale = 1.0f;
	//int lastMeasure = -1;

	float group_hz = 0.0f;
	float group_attack = 0.0f;
	float group_decay = 0.0f;
	float group_sustain = 0.0f;
	float group_release = 0.0f;
	VisEnvelope visEnv;
	array<float, 1024> visBuffer;
	bool editName = false;
	array<char, 64> textbuf;
	int copyIdx = -1;
	vector<string> measureList;
	vector<string> measureTypes = { "Empty", "Step", "Midi", "Life" };
	Measure::Type currentMeasureType = Measure::Type::EMPTY;
	int currentSampleIdx = 0;
	bool lockEditingBlock = true;
	int activeChannel = 0;
	int tmp = 0;
	bool editModMatrixOptions = false;

	vector<string> scaleNames = { "Major", "Natural Minor", "Harmonic Minor", "Melodic Minor", "Ionian", "Dorian", "Phrygian", "Lydian", "Myxolydian", "Aeolian", "Locrian" };
	int currentScaleType = 0;
	vector<string> chordTypeNames = { "Diminished", "Half Dimished", "Minor", "Minor-Major", "Dominant", "Major", "Augmented", "Augmented Major", "Sus2", "Sus4" };
	int currentChordType = 0;
	vector<string> octaveNames = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
	vector<string> noteNames = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
	//Zieverink red • orange • yellow / white • yellow(Cincinnati Contemporary Art Center)
	//const vector< ImVec4 > noteColours = {
	//	(ImVec4)ImColor(154,205,50),
	//	(ImVec4)ImColor(0, 128, 0),
	//	(ImVec4)ImColor(13, 152, 186),
	//	(ImVec4)ImColor(0,0,205),
	//	(ImVec4)ImColor(75,0,130),
	//	(ImVec4)ImColor(238, 130, 238),
	//	(ImVec4)ImColor(95, 75, 139),
	//	(ImVec4)ImColor(200, 37, 54),
	//	(ImVec4)ImColor(205, 0, 0),
	//	(ImVec4)ImColor(255, 165, 0),
	//	(ImVec4)ImColor(255,255,224),
	//	(ImVec4)ImColor(255,255,0)
	//};
	const vector<float> noteHues = {
		1.39626f, 2.0944f,3.35103f,4.18879f,4.79966f,5.23599f,4.5204f,6.17847f,0.0f,0.680678f,1.0472f,1.0472f
	};

	const vector<float> channelHues = {
		0.125f + (0.125f * 0),
		0.125f + (0.125f * 1),
		0.125f + (0.125f * 2),
		0.125f + (0.125f * 3),
		0.125f + (0.125f * 4),
		0.125f + (0.125f * 5),
		0.125f + (0.125f * 6),
		0.125f + (0.125f * 7),
	};
	
	vector<string> step_event_names = { "Lane Trigger", "Lane End of Cycle", "Clock" };
	StepSequencer::Events current_step_event = StepSequencer::Events::SEQUENCE_TRIGGER;
	vector<string> step_action_names = { "Swap", "Rotate Fwd", "Rotate Bck", "Change Clock", "Jog", "Pause", "Randomize", "Reset", "Enable Rule", "Disable Rule", "Toggle Rule" };
	StepSequencer::Actions current_step_action = StepSequencer::Actions::SWAP;


	vector<string> kit_event_names = { "Gate Count", "Tick Count" };
	Kit::Events current_kit_event = Kit::Events::GATES;
	vector<string> kit_action_names = { "Load Kit", "Swap Notes", "Swap Samples","Rotate Notes Fwd", "Rotate Notes Bck", "Rotate Samples Fwd", "Rotate Samples Bck","Turing Notes (kit)", "Transpose Notes", "Transpose Octaves", "Enable Rule", "Disable Rule", "Toggle Rule" };
	Kit::Actions current_kit_action = Kit::Actions::SWAP_NOTES;

	
	SeriesGfx(Series* audioptr) {

		assert(audioptr != nullptr);
		audio = audioptr;

		for (auto& v : visBuffer) {
			v = 0.0f;
		}
		visEnv.dt = 1.0f / 1024.0f;

		for (auto& v : textbuf) {
			v = 0;
		}

		measureList.reserve(65);
		measureList.push_back("None");

		for (int i = 0; i < 64; i++) {
			measureList.push_back(to_string(i));
		}

	}


	//
	//
	// Clock Gui
	//
	//


	void clockGUI() {
		ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_FirstUseEver);
		ImGui::Begin("masterClock", &showClock);
		ImGui::PushID("masterClock");

		ImGui::DragFloat("BPM", &audio->clock->bpm, 1.0f, 40, 240, "%.0f");

		ImGui::PopID();
		ImGui::End();
	}




	//
	//
	// Mixer Gui
	//
	//
	void mixerGUI() {
		
		ImGui::PushStyleColor(ImGuiCol_TitleBg, (ImVec4)ImColor::HSV(channelHues[activeChannel], 0.5f, 0.5f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_TitleBgActive, (ImVec4)ImColor::HSV(channelHues[activeChannel], 0.5f, 0.5f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, (ImVec4)ImColor::HSV(channelHues[activeChannel], 0.5f, 0.5f, 0.2f));

		ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_FirstUseEver);
		//ImGui::SetNextWindowContentWidth(1760.0);
		ImGui::Begin("Master Mixer", &showMixer, ImGuiWindowFlags_HorizontalScrollbar);
				
		ImGui::PushID("mixer");

		ImGui::Columns(audio->channels.size() +1, "channels", false);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));


		for (int c = 0; c < audio->channels.size(); c++) {

			if (c == activeChannel) {
				ImGui::Text("Active");
			}
			else {
				ImGui::Text(to_string(c).c_str());
			}
			
			
			
			ImGui::NewLine();
			ImGui::PushID(c);

			ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor::HSV(0.125f + (0.125f * c), 0.5f, 0.5f));
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4)ImColor::HSV(0.125f + (0.125f * c), 0.6f, 0.5f));
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive, (ImVec4)ImColor::HSV(0.125f + (0.125f * c), 0.7f, 0.5f));
			ImGui::PushStyleColor(ImGuiCol_SliderGrab, (ImVec4)ImColor::HSV(0.125f + (0.125f * c), 0.9f, 0.9f));

			ImGui::PushID(0);
			ImGui::VSliderFloat("", ImVec2(18, 160), &audio->channels[c]->eq.high.gain, -30.0f, 30.0f); ImGui::SameLine();
			if (ImGui::IsItemActive() || ImGui::IsItemHovered())
				ImGui::SetTooltip("High Gain %.3f", audio->channels[c]->eq.high.gain);
			ImGui::PopID();
			ImGui::PushID(1);
			ImGui::VSliderFloat("", ImVec2(18, 160), &audio->channels[c]->eq.highMidHz, 400.0f, 8000.0f); ImGui::SameLine();
			if (ImGui::IsItemActive() || ImGui::IsItemHovered())
				ImGui::SetTooltip("High Mid Hz %.3f", audio->channels[c]->eq.highMidHz);
			ImGui::PopID();
			ImGui::PushID(2);
			ImGui::VSliderFloat("", ImVec2(18, 160), &audio->channels[c]->eq.highMid.gain, -30.0f, 30.0f); ImGui::SameLine();
			if (ImGui::IsItemActive() || ImGui::IsItemHovered())
				ImGui::SetTooltip("High Mid Gain %.3f", audio->channels[c]->eq.highMid.gain);
			ImGui::PopID();
			ImGui::PushID(3);
			ImGui::VSliderFloat("", ImVec2(18, 160), &audio->channels[c]->eq.lowMidHz, 100.0f, 2000.0f); ImGui::SameLine();
			if (ImGui::IsItemActive() || ImGui::IsItemHovered())
				ImGui::SetTooltip("Low Mid Hz %.3f", audio->channels[c]->eq.lowMidHz);
			ImGui::PopID();
			ImGui::PushID(4);
			ImGui::VSliderFloat("", ImVec2(18, 160), &audio->channels[c]->eq.lowMid.gain, -30.0f, 30.0f); ImGui::SameLine();
			if (ImGui::IsItemActive() || ImGui::IsItemHovered())
				ImGui::SetTooltip("Low Mid Gain %.3f", audio->channels[c]->eq.lowMid.gain);
			ImGui::PopID();
			ImGui::PushID(5);
			ImGui::VSliderFloat("", ImVec2(18, 160), &audio->channels[c]->eq.level, 0.0f, 1.0f);
			if (ImGui::IsItemActive() || ImGui::IsItemHovered())
				ImGui::SetTooltip("Level %.3f", audio->channels[c]->eq.level);
			ImGui::PopID();

			ImGui::PopStyleColor(4);

			ImGui::NewLine();
			ImGui::PushID(c);
			ImGui::Checkbox("Running", &audio->channels[c]->running);
			ImGui::PopID();

			ImGui::PushID("SetMeasure");
			
			if (ImGui::Button("Measure: ")) {
				ImGui::OpenPopup("select_measure_popup");
			}

			ImGui::SameLine();
			ImGui::Text(to_string(audio->channels[c]->measureRunner->measure_idx).c_str());

			if (ImGui::BeginPopup("select_measure_popup")) {
				//ImGui::PushID("measures");

				for (int i = 0; i < audio->channels[c]->measures->size(); i++) {
					//ImGui::PushID(i);
					if (ImGui::Button(to_string(i).c_str(), ImVec2(20.0, 20.0))) {
						audio->channels[c]->loadMeasure(i);
						ImGui::CloseCurrentPopup();
					}
					//ImGui::PopID();
					if ((i % 8) < 7) ImGui::SameLine();

				}
				//ImGui::PopID();

				ImGui::EndPopup();
			}
			
			ImGui::PopID();


			ImGui::PushID("SetKit");

			if (ImGui::Button("Kit: ")) {
				ImGui::OpenPopup("select_kit_popup");
			}

			ImGui::SameLine();
			ImGui::Text(to_string(audio->channels[c]->currentKit).c_str());

			if (ImGui::BeginPopup("select_kit_popup")) {

				for (int i = 0; i < audio->channels[c]->kitManager->size(); i++) {
					if (ImGui::Button(to_string(i).c_str(), ImVec2(20.0, 20.0))) {
						audio->channels[c]->loadKit(i);
						ImGui::CloseCurrentPopup();
					}
					if ((i % 8) < 7) ImGui::SameLine();
				}
				ImGui::EndPopup();
			}

			ImGui::PopID();

			ImGui::NewLine();

			if (ImGui::Button("Set Sampler")) {
				audio->channels[c]->setEngine(0); //Samples!
			}

			if (ImGui::Button("Set WT Voice")) {
				audio->channels[c]->setEngine(1);
			}

			if (ImGui::Button("Set Modal Voice")) {
				audio->channels[c]->setEngine(2);
			}

			if (ImGui::Button("Set Slicer")) {
				audio->channels[c]->setEngine(3);
			}

			if (ImGui::Button("Set Block")) {
				audio->channels[c]->setEngine(4);
			}

			if (ImGui::Button("Set FmWT")) {
				audio->channels[c]->setEngine(5);
			}

			if (ImGui::Button("Set Multi Voice")) {
				audio->channels[c]->setEngine(6);
			}

			if (ImGui::Button("Set Sfx Player")) {
				audio->channels[c]->setEngine(7);
			}

			if (ImGui::Button("Unload Engine")) {
				audio->channels[c]->disable_engine();
			}

			ImGui::PopID();
			ImGui::NextColumn();
		}

		ImGui::PushID("master");
		ImGui::Text("Master");
		ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor::HSV(0.0f, 0.5f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_SliderGrab, (ImVec4)ImColor::HSV(0.0f, 0.9f, 0.9f));
		ImGui::VSliderFloat("##v", ImVec2(18, 160), &audio->masterLevel, 0.0f, 1.0f, "");
		if (ImGui::IsItemActive() || ImGui::IsItemHovered())
			ImGui::SetTooltip("%.3f", audio->masterLevel);
		ImGui::PopStyleColor(4);
		ImGui::PopID();

		ImGui::Columns(1);
	
		ImGui::PopID();
		
		ImGui::PopStyleVar();

		
		ImGui::End();
		ImGui::PopStyleColor(3);
		
	}









	//
	//
	// Audio Engine Gui
	//
	//
	//Change to current channel sometime
	void channelEngineGUI() {

		ImGui::PushStyleColor(ImGuiCol_TitleBg, (ImVec4)ImColor::HSV(channelHues[activeChannel], 0.5f, 0.5f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_TitleBgActive, (ImVec4)ImColor::HSV(channelHues[activeChannel], 0.5f, 0.5f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, (ImVec4)ImColor::HSV(channelHues[activeChannel], 0.5f, 0.5f, 0.2f));

		ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_FirstUseEver);
		ImGui::Begin("Audio Engine", &showMixer, ImGuiWindowFlags_HorizontalScrollbar);

		if (audio->channels[activeChannel]->audioEngine) {
			ImGui::Checkbox("Link Editing Parameters", &lockEditingBlock);
			
			const string running_type = audio->channels[activeChannel]->audioEngine->myType();
			
			//Sadly we need to do some aditional stuff for slicer ftm
			//if (running_type.compare("struct Slicer") == 0) {
			//	ImGui::PushID("Slicer");
			//
			//	const auto slicer = dynamic_pointer_cast<Slicer, EngineUnit>(audio->channels[activeChannel]->audioEngine);
			//	if (slicer->getSampleData()) {
			//		ImGui::PlotLines("###Current Slice", slicer->getSampleData()->data(), slicer->getSampleSize(), 0, slicer->getSampleName().c_str(), -1.0f, 1.0f, ImVec2(200, 75));
			//	}
			//	else {
			//		const float blank[1] = { 0.0f };
			//		ImGui::PlotLines("###Current Slice", blank, 1, 0, "", -1.0f, 1.0f, ImVec2(200, 75));
			//	}
			//		
			//	if (ImGui::IsItemClicked()) {
			//		ImGui::OpenPopup("chance_slice");
			//	}
			//	if (ImGui::BeginPopup("chance_slice"))
			//	{
			//		for (int s = 0; s < audio->sampleManager->getSampleList().size(); s++) {
			//			if (ImGui::Selectable(audio->sampleManager->getSampleList()[s].c_str())) {
			//				slicer->setSample(audio->sampleManager->getSample(s));
			//				ImGui::CloseCurrentPopup();
			//			}
			//		}
			//		ImGui::EndPopup();
			//	}
			//
			//
			//	if (slicer->getSampleData()) {
			//		for (int i = 0; i < POLY; i++) { 
			//			const auto start = slicer->slice_start[i];
			//			const int length = static_cast<int>((slicer->slice_start[i + 1] - start) * slicer->trim_length[i]);
			//
			//			ImGui::PlotLines("", slicer->getSampleData()->data() + start, length, 0, "", -1.0f, 1.0f, ImVec2(50, 50));
			//			if (i < 7)
			//				ImGui::SameLine();
			//		}
			//
			//		ostringstream slabel;
			//		slabel.str(string());
			//		slabel << "Length 0";
			//		
			//		ImGui::SliderFloat(slabel.str().c_str(), &slicer->trim_length[0], 0.0f, 1.0f);
			//
			//		for (int i = 1; i < POLY; i++) { 
			//			ImGui::PushID(i);
			//
			//			slabel.str(string());
			//			slabel << "Slice " << i;
			//			ImGui::LimitInputsize_t(slabel.str().c_str(), &slicer->slice_start[i], slicer->slice_start[i - 1], slicer->slice_start[i + 1]);
			//
			//			slabel.str(string());
			//			slabel << "Length " << i;
			//			ImGui::SliderFloat(slabel.str().c_str(), &slicer->trim_length[i], 0.0f, 1.0f);
			//
			//			ImGui::PopID();
			//		}
			//	}
			//
			//	ImGui::PopID();
			//
			//}

			if (running_type.compare("struct MultiVoice") == 0) {

				const auto multi = dynamic_pointer_cast<MultiVoice, EngineUnit>(audio->channels[activeChannel]->audioEngine);

				if (ImGui::Button("Prev Engine")) {
					multi->prev_voice();
				}
				ImGui::Text("Current Engine: %d", multi->active_engine());
				ImGui::SameLine();
				if (ImGui::Button("Next Engine")) {
					multi->next_voice();
				}
			}
					   			 		  
			
			auto engineOptions = audio->channels[activeChannel]->audioEngine->getOptions();

			ImGui::PushID("Options");
			int tmp = 0;
			for (auto& o : engineOptions) {
					if (!lockEditingBlock)
						ImGui::PushID(tmp);
					ImGui::SliderFloat(o.name.c_str(), o.resP, o.min, o.max);
					if (!lockEditingBlock)
						ImGui::PopID();
					tmp++;
			}
			ImGui::PopID();

			auto engineConsumers = audio->channels[activeChannel]->audioEngine->getInterface();
			for (auto& c : engineConsumers) {

				ImGui::PushID("Consumers");
				
				for (int i = 0; i < POLY; i++) {
					if (!lockEditingBlock)
						ImGui::PushID(i);
					ImGui::SliderFloat(c.name.c_str(), &c.resP[i], c.min, c.max);
					if (!lockEditingBlock)
						ImGui::PopID();
				}
				
				ImGui::PopID();
			}
			
			ImGui::PlotLines("Debug Scope", &audio->channels[activeChannel]->audioEngine->shittyscope[0], static_cast<int>(scopeScale * audio->channels[activeChannel]->audioEngine->shittyscope.size()), 0, "Sample", -1.0f, 1.0f, ImVec2(200, 100));
			ImGui::SliderFloat("Scope Scale", &scopeScale, 0.1, 1.0f);


		}

		ImGui::End();
		ImGui::PopStyleColor(3);


	}



	//
	//
	// Measures Gui
	//
	// width min 575
	// Rules * 300
	void channelMeasureGUI() {

		ImGui::PushStyleColor(ImGuiCol_TitleBg, (ImVec4)ImColor::HSV(channelHues[activeChannel], 0.5f, 0.5f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_TitleBgActive, (ImVec4)ImColor::HSV(channelHues[activeChannel], 0.5f, 0.5f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, (ImVec4)ImColor::HSV(channelHues[activeChannel], 0.5f, 0.5f, 0.2f));

		const auto stepwidth = max<int>(575, 63 + audio->channels[activeChannel]->measureRunner->getMaxStepLength() * 32);
		const auto rulewidth = max<int>(575, audio->channels[activeChannel]->measureRunner->step->rules.size() * 300); 
		//Will need expanding when measures get rules of their own
		//sub windows?

		ImGui::SetNextWindowContentWidth(max<int>(stepwidth, rulewidth));

		ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_FirstUseEver);
		ImGui::Begin("Measure", &showClock, ImGuiWindowFlags_HorizontalScrollbar);
		ImGui::PushID("measure");

		ImGui::PushItemWidth(100);


		//if (currentMeasure != lastMeasure) {
		//	lastMeasure == currentMeasure;
		//	currentMeasureType = audio->measures->measures[currentMeasure]->is();
		//}
		

		ImGui::Text("Current Measure: "); ImGui::SameLine(); ImGui::Text(to_string(audio->channels[activeChannel]->measureRunner->measure_idx).c_str());
		if (audio->channels[activeChannel]->measureRunner->measure_idx >= 0) {
			ImGui::SameLine();
			if (ImGui::Button("Save")) {
				audio->channels[activeChannel]->measureRunner->saveMeasure();
			}
			ImGui::SameLine();

			ImGui::PushID("SaveAs");

			if (ImGui::Button("Save As")) {
				ImGui::OpenPopup("saveas_measure_popup");
			}

			if (ImGui::BeginPopup("saveas_measure_popup")) {

				for (int i = 0; i < audio->measures->size(); i++) {
					if (ImGui::Button(to_string(i).c_str(), ImVec2(20.0, 20.0))) {
						audio->channels[activeChannel]->measureRunner->saveMeasure(i);
						ImGui::CloseCurrentPopup();
					}
					if ((i % 8) < 7) ImGui::SameLine();
				}
				ImGui::EndPopup();
			}

			ImGui::PopID();

			currentMeasureType = audio->channels[activeChannel]->measureRunner->is();

			if (ImGui::Combo2("Measure Type", reinterpret_cast<int*>(&currentMeasureType), measureTypes)) {
				audio->channels[activeChannel]->measureRunner->setType(currentMeasureType);
			}

			if (currentMeasureType == Measure::Type::EMPTY) {

				for (int c = 0; c < POLY; c++) { 
					ImGui::PushID(c);
					ImGui::SliderFloat(to_string(c).c_str(), &audio->channels[activeChannel]->measureRunner->empty->gates[c], 0.0f, 1.0f);
					ImGui::PopID();
				}

			}

			if (currentMeasureType == Measure::Type::MIDI) {
				ImGui::Text("Wip");
			}


			if (currentMeasureType == Measure::Type::STEP) {

				if (ImGui::IsKeyPressed(321, false))  //kp-1
					audio->channels[activeChannel]->measureRunner->step->toggleCurrentStep(0);
				if (ImGui::IsKeyPressed(322, false))  //kp-2
					audio->channels[activeChannel]->measureRunner->step->toggleCurrentStep(1);
				if (ImGui::IsKeyPressed(323, false))  //kp-3
					audio->channels[activeChannel]->measureRunner->step->toggleCurrentStep(2);
				if (ImGui::IsKeyPressed(324, false))  //kp-4
					audio->channels[activeChannel]->measureRunner->step->toggleCurrentStep(3);
				if (ImGui::IsKeyPressed(325, false))  //kp-5
					audio->channels[activeChannel]->measureRunner->step->toggleCurrentStep(4);
				if (ImGui::IsKeyPressed(326, false))  //kp-6
					audio->channels[activeChannel]->measureRunner->step->toggleCurrentStep(5);
				if (ImGui::IsKeyPressed(327, false))  //kp-7
					audio->channels[activeChannel]->measureRunner->step->toggleCurrentStep(6);
				if (ImGui::IsKeyPressed(328, false))  //kp-8
					audio->channels[activeChannel]->measureRunner->step->toggleCurrentStep(7);


				ImGui::SliderFloat("Swing", &audio->channels[activeChannel]->measureRunner->step->clockProvider->swing, -1.0f, 1.0f);

				for (int l = 0; l < POLY; l++) {  
					const int c = audio->channels[activeChannel]->measureRunner->step->getMappedLane(l);
					ImGui::PushID(c);
					currentClockSource = audio->channels[activeChannel]->measureRunner->step->getLaneClockType(c);
					if (ImGui::Combo2("Clock Provider", reinterpret_cast<int*>(&currentClockSource), clockSourceNames)) {
						//laneList
						audio->channels[activeChannel]->measureRunner->step->setLaneClockType(c, currentClockSource);
					}

					ImGui::SameLine();

					currentSequenceType = audio->channels[activeChannel]->measureRunner->step->getLaneType(c);

					if (ImGui::Combo2("Sequence Type", reinterpret_cast<int*>(&currentSequenceType), stepSequenceTypeNames)) {
						audio->channels[activeChannel]->measureRunner->step->setLaneType(c, currentSequenceType);
						audio->channels[activeChannel]->measureRunner->step->restart(); //Sync up the channels
																					//A convienience for editing, and to cover the lack we're not currently saving current steps
																					//Note when implementing jog just save the current step offset
																					//So removing this here will result in more happenstance

						//Another editing convienice, presumably if you are changing the type you want some sort of signal out
						if (audio->channels[activeChannel]->measureRunner->step->getLaneClockType(c) == ClockProvider::ClockTypes::NONE) {
							audio->channels[activeChannel]->measureRunner->step->setLaneClockType(c, ClockProvider::ClockTypes::QUARTER);
							currentClockSource = ClockProvider::ClockTypes::QUARTER;
						}
					}

					auto channelType = audio->channels[activeChannel]->measureRunner->step->getLaneType(c);


					//Shame about the dynamic pointer cast schenanigans, however hopefully they won't be needed much outside of a gui context
					switch (channelType) {

					case StepSequencer::Types::NONE:
						ImGui::Text("None");
						break;

					case StepSequencer::Types::STEP:
					{
						auto stepSeq = dynamic_pointer_cast<StepSequence, StepperBase>(audio->channels[activeChannel]->measureRunner->step->getLaneStepper(c));


						ImGui::SameLine();
						ImGui::LimitInputsize_t("Length", &stepSeq->pattern.length, 1, 256);

						ImGui::Text("Step");
						ImGui::SameLine();

						for (int i = 0; i < stepSeq->pattern.length; i++) {
							const auto v = i % 4;

							ImGui::PushID(i);

							//This is so that if current step changes mid loop PopStyleColor will always get called
							if (i == stepSeq->pattern.getCurrentStep()) {
								ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
							}
							else {
								ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_Button));
							}

							if (stepSeq->pattern.pattern[i]) {
								if (ImGui::Button("On", ImVec2(20.0f, 20.0f))) {
									stepSeq->pattern.pattern[i] = 0;
								}
							}
							else {
								if (ImGui::Button(to_string(v).c_str(), ImVec2(20.0f, 20.0f))) {
									stepSeq->pattern.pattern[i] = 1;
								}
							}

							if (v == 3) {
								ImGui::SameLine();
								ImGui::InvisibleButton("", ImVec2(10.0f, 20.0f));
							}

							ImGui::PopStyleColor();

							ImGui::PopID();
							ImGui::SameLine();

						}
						//ImGui::PushItemWidth(100);
					}
					break;

					case StepSequencer::Types::EUCLIDIAN:
					{
						ImGui::Text("Euclidian");
						ImGui::SameLine();

						auto ed = dynamic_pointer_cast<EuclidianDivider, StepperBase>(audio->channels[activeChannel]->measureRunner->step->getLaneStepper(c));

						ImGui::DragInt("Fill", &ed->fill, 0, 256);
						ImGui::SameLine();
						ImGui::DragSize_t("Length", &ed->length, 0, 256);
						ImGui::SameLine();
						ImGui::SliderFloat("Chance", &ed->chance, 0.0f, 1.0f);
						ImGui::SameLine();

						if (audio->channels[activeChannel]->measureRunner->gates[c]) {
							ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
							ImGui::Button("Out", ImVec2(28.0f, 20.0f));
							ImGui::PopStyleColor();
						}
						else {
							ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_Button));
							ImGui::Button("Out", ImVec2(28.0f, 20.0f));
							ImGui::PopStyleColor();
						}
					}
					break;
					}


					ImGui::PopID();
					ImGui::NewLine();
				}
				ImGui::PopItemWidth();


				ImGui::Separator();
				ImGui::Text("Rules:");
				ImGui::PushID("Rules Editing");
				if (ImGui::Button("Disable Rules")) {
					for (auto& r : audio->channels[activeChannel]->measureRunner->step->rules) {
						r.run = false;
						r.event_counter = 0;
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("Reset Lanes and Rules")) {
					//Will need to switch on type
					audio->channels[activeChannel]->measureRunner->step->restart();
				}
				ImGui::SameLine();
				if (ImGui::Button("Add Rule")) {
					audio->channels[activeChannel]->measureRunner->step->rules.push_back(Rule<StepSequencer::Events, StepSequencer::Actions>());
				}

				ImGui::Columns(audio->channels[activeChannel]->measureRunner->step->rules.size() + 1);
				int tmp = 0;
				for (auto& r : audio->channels[activeChannel]->measureRunner->step->rules) {
					ImGui::PushID(tmp++);

					ImGui::Checkbox("Run", &r.run);
					current_step_event = r.event;
					if (ImGui::Combo2("Type", reinterpret_cast<int*>(&current_step_event), step_event_names)) {
						//laneList
						r.event = current_step_event;
					}

					if (r.event == StepSequencer::Events::CLOCK) {
						currentClockSource = static_cast<ClockProvider::ClockTypes>(r.event_src);
						if (ImGui::Combo2("Clock Provider", reinterpret_cast<int*>(&currentClockSource), clockSourceNames)) {
							//laneList
							r.event_src = static_cast<int>(currentClockSource);
						}
					}
					else {
						ImGui::SliderInt("Event Src", &r.event_src, 0, POLY-1);
					}

					ImGui::DragInt("Action Count", &r.action_threashold);

					current_step_action = r.action;
					if (ImGui::Combo2("action", reinterpret_cast<int*>(&current_step_action), step_action_names)) {
						//laneList
						r.action = current_step_action;
					}

					ImGui::SliderInt("Action Src", &r.action_src, 0, POLY - 1);

					ImGui::SliderInt("Action Dst", &r.action_dst, 0, POLY - 1);
					ImGui::PopID();
					ImGui::NextColumn();
				}
				ImGui::Columns(1);
				ImGui::PopID();

			}





			if (currentMeasureType == Measure::Type::LIFE) {
				ImGui::PushID("Life");

				currentClockSource = audio->channels[activeChannel]->measureRunner->life->getEvolveClockType();
				if (ImGui::Combo2("Evolve Clock Provider", reinterpret_cast<int*>(&currentClockSource), clockSourceNames)) {
					//laneList
					audio->channels[activeChannel]->measureRunner->life->setEvolveClockType(currentClockSource);
				}
				ImGui::SameLine();
				currentClockSource = audio->channels[activeChannel]->measureRunner->life->getStepClockType();
				if (ImGui::Combo2("Step Clock Provider", reinterpret_cast<int*>(&currentClockSource), clockSourceNames)) {
					//laneList
					audio->channels[activeChannel]->measureRunner->life->setStepClockType(currentClockSource);
				}

				ImGui::Separator();

				for (int j = 0; j < POLY; j++) {       
					for (int i = 0; i < POLY; i++) {	

						if (i == audio->channels[activeChannel]->measureRunner->life->getCurrentStep()) {
							ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
						}
						else {
							ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_Button));
						}

						ImGui::PushID(i + j * POLY);
						if (audio->channels[activeChannel]->measureRunner->life->getLife(i, j)) {
							if (ImGui::Button("X")) {
								audio->channels[activeChannel]->measureRunner->life->toggleLife(i, j);
							}
						}
						else {
							if (ImGui::Button("O")) {
								audio->channels[activeChannel]->measureRunner->life->toggleLife(i, j);
							}
						}
						ImGui::PopID();
						ImGui::PopStyleColor();

						ImGui::SameLine();


					}

					ImGui::NewLine();

				}
				ImGui::PopID();
			}
		}
		ImGui::PopID();
		ImGui::End();

		ImGui::PopStyleColor(3);
	}



	//
	//
	// Modulation Gui
	//
	//

	//Maybe I should include S&H for a constant ish thing
	void modulationGUI() {
		ImGui::PushStyleColor(ImGuiCol_TitleBg, (ImVec4)ImColor::HSV(channelHues[activeChannel], 0.5f, 0.5f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_TitleBgActive, (ImVec4)ImColor::HSV(channelHues[activeChannel], 0.5f, 0.5f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, (ImVec4)ImColor::HSV(channelHues[activeChannel], 0.5f, 0.5f, 0.2f));


		ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_FirstUseEver);
		ImGui::Begin("Modulation", &showClock);
		ImGui::PushID("modulation");

		ImGui::Checkbox("Link Editing Parameters", &lockEditingBlock);



		ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor::HSV(channelHues[activeChannel], 0.5f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4)ImColor::HSV(channelHues[activeChannel], 0.6f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, (ImVec4)ImColor::HSV(channelHues[activeChannel], 0.7f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_SliderGrab, (ImVec4)ImColor::HSV(channelHues[activeChannel], 0.9f, 0.9f));
		ImGui::PushStyleColor(ImGuiCol_PlotLines, (ImVec4)ImColor::HSV(channelHues[activeChannel], 0.7f, 0.5f));

		ImGui::PushID(activeChannel);
		const int offset = activeChannel * CHANNELS;

		bool update = false;
		ImGui::Text("Channel: "); ImGui::SameLine(); ImGui::Text(to_string(activeChannel).c_str());
		ImGui::Separator();
		auto modConsumers = audio->channels[activeChannel]->modulation->getInterface();
		int tmp = 0;
		for (auto& c : modConsumers) {

			ImGui::PushID(tmp);

			for (int i = 0; i < POLY; i++) {
				if(!lockEditingBlock)
					ImGui::PushID(i);
				ImGui::SliderFloat(c.name.c_str(), &c.resP[i], c.min, c.max);
				if (!lockEditingBlock)
					ImGui::PushID(i);
			}

			ImGui::PopID();
			tmp++;
		}

		//Just show what the 1st envelope would look like
		visEnv.attackTime = audio->channels[activeChannel]->modulation->getInterface()[1].resP[0];
		visEnv.decayTime = audio->channels[activeChannel]->modulation->getInterface()[2].resP[0];
		visEnv.sustainLevel = audio->channels[activeChannel]->modulation->getInterface()[3].resP[0];
		visEnv.releaseTime = audio->channels[activeChannel]->modulation->getInterface()[4].resP[0];

		visEnv.trigger();
		for (int i = 0; i < visBuffer.size(); i++) {
			if (i < visBuffer.size() / 2) {
				visBuffer[i] = visEnv.process(1.0f);
			}
			else {
				visBuffer[i] = visEnv.process(0.0f);
			}
		}

		
		
		ImGui::PlotLines("", visBuffer.data(), visBuffer.size(), 0, "", 0.0f, 1.0f, ImVec2(180.0f, 50.0f));
		
		ImGui::NewLine();

		ImGui::PopID();
		ImGui::PopStyleColor(5);


		

		ImGui::PopID();
		ImGui::End();

		ImGui::PopStyleColor(3);

	}

	//
	//
	// Mod Matrix Gui
	//
	//

	//Break consumers / provders up by channel
	//Rather then just blasting out the consumer & provider lists
	void modMatrixGUI() {
		
		ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_FirstUseEver);
		//ImGui::SetNextWindowContentWidth((1 + consumers.size()) * 120.0f);
		ImGui::Begin("Modulation Matrix", &showModMatrix, ImGuiWindowFlags_HorizontalScrollbar);
		ImGui::PushID("ModMatrix");

		ImGui::SliderFloat("Frame Slew in s", &audio->modMatrix->frameSlewTime, 0.0f, 100.0f);
		ImGui::SameLine();
		ImGui::Checkbox("Edit Options", &editModMatrixOptions);

		ImGui::Text("Current Frame: %d", audio->modMatrix->getCurrentFrame());
		ImGui::Text("Name: "); ImGui::SameLine();
		if (editName) {
			fill(textbuf.begin(), textbuf.end(), 0);
			for (int i = 0; i < audio->modMatrix->connectionSet[audio->modMatrix->getCurrentFrame()].name.size(); i++) {
				if (i < textbuf.size()) {
					textbuf[i] = audio->modMatrix->connectionSet[audio->modMatrix->getCurrentFrame()].name[i];
				}
			}
			
			if (ImGui::InputText("Edit Name", textbuf.data(), textbuf.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
				audio->modMatrix->connectionSet[audio->modMatrix->getCurrentFrame()].name = textbuf.data();
				editName = false;
			}
		} else {
			if (ImGui::Selectable(audio->modMatrix->connectionSet[audio->modMatrix->getCurrentFrame()].name.c_str())) {
				editName = true;
			}
		}

		ImGui::DragSize_t("Matrix Frames", &audio->modMatrix->length, 1, 64);
		
		if (ImGui::Button("Previous Frame")) {
			audio->modMatrix->prevFrame();
		}
		ImGui::SameLine();
		if (ImGui::Button("Next Frame")) {
			audio->modMatrix->nextFrame();
		}
		ImGui::SameLine();

		if (ImGui::Button("Copy")) {
			copyIdx = audio->modMatrix->getCurrentFrame();
		}

		if (ImGui::Button("Paste")) {

			
			while (audio->modMatrix->busy()) {
				std::this_thread::sleep_for(std::chrono::microseconds(100)); //was milliseconds(1)
			}

			if (copyIdx >= 0) {
				audio->modMatrix->pasteCurrentFrame(copyIdx);
				copyIdx = -1;
			}
			
		}

		//
		//Big page of buttons starts here
		//
		ImGui::Separator();
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
		auto posCache = ImGui::GetCursorPos();
		posCache.x = posCache.x + 120.0f;
		ImGui::SetCursorPos(posCache);

		if (editModMatrixOptions) {
			ImGui::SetNextWindowContentWidth(max<float>(120.0f, audio->modMatrix->options.size() * 120.0f));
			float w = ImGui::GetWindowWidth();
			ImGui::BeginChild("OptionNames", ImVec2(w - 120.0f - 25.0f, ImGui::GetFontSize() + 14.0f), true, ImGuiWindowFlags_NoScrollbar);
			ImGui::Columns(max<int>(1, audio->modMatrix->options.size()), "options");

			for (auto& opt : audio->modMatrix->options) {
				ImGui::Text(opt->name.c_str()); ImGui::NextColumn();
			}


			auto columnsScrollM = ImGui::GetScrollMaxX();
			ImGui::Columns(1);
			ImGui::EndChild();
			posCache = ImGui::GetCursorPos();
			ImGui::PopStyleVar(2);


			if (audio->modMatrix->providers.size() > 0) {
				float scrollx = 0.0f;
				float scrolly = 0.0f;

				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
				ImGui::BeginChild("ProviderNames", ImVec2(120.0f, 0.0f), true, ImGuiWindowFlags_NoScrollbar);

				for (auto& pep : audio->modMatrix->providers) {
					if (pep->size > 1)
						continue;

					ImGui::Text(pep->name.c_str());
					ImGui::SameLine();
					ImGui::InvisibleButton("", ImVec2(1.0f, 19.0f));
					ImGui::Separator();
				}

				ImGui::InvisibleButton("", ImVec2(1.0f, 25.0f));
				auto providerHeight = ImGui::GetWindowHeight();
				ImGui::EndChild();
				ImGui::PopStyleVar(2);

				posCache.x = posCache.x + 120.0f;
				ImGui::SetCursorPos(posCache);


				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
				ImGui::SetNextWindowContentWidth(max<float>(120.0f, (audio->modMatrix->options.size() * 120.0f)));
				ImGui::BeginChild("ConnectionTable", ImVec2(0, providerHeight), true, ImGuiWindowFlags_HorizontalScrollbar);


				if (audio->modMatrix->options.size() > 0) {
					ImGui::Columns(audio->modMatrix->options.size(), "options");
				}

				for (auto& pep : audio->modMatrix->providers) {

					if (pep->size > 1)   //
						continue;

					for (auto& oep : audio->modMatrix->options) {
						const auto k = audio->modMatrix->key(reinterpret_cast<uintptr_t>(pep), reinterpret_cast<uintptr_t>(oep));

						ImGui::PushID(k);

						bool found = false;
						for (auto& c : audio->modMatrix->connectionSet[audio->modMatrix->getCurrentFrame()].connections) {
							if (c.provider == pep && c.consumer == oep) {
								found = true;

								ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.1783f, 0.6f, 0.6f));
								ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.1783f, 0.7f, 0.7f));
								ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.1783f, 0.8f, 0.8f));

								if (ImGui::Button("Set", ImVec2(40, 19))) {
									audio->modMatrix->setConnection(audio->modMatrix->getCurrentFrame(), pep, oep);
								}

								ImGui::SameLine();
								ImGui::PopStyleColor(3);
								ImGui::DragFloat("", &c.targetAttenuation, 0.001f, 0.0f, 1.0f);
							}

						}

						if (found == false) {

							ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.654f, 0.6f, 0.6f));
							ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.654f, 0.7f, 0.7f));
							ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.654f, 0.8f, 0.8f));

							if (ImGui::Button("UnSet", ImVec2(40, 19))) {
								audio->modMatrix->setConnection(audio->modMatrix->getCurrentFrame(), pep, oep);
							}

							ImGui::PopStyleColor(3);

						}


						ImGui::NextColumn();
						ImGui::PopID();

					}

					ImGui::Separator();
				}

				auto csMax = ImGui::GetScrollMaxX();

				scrollx = ImGui::GetScrollX();
				scrolly = ImGui::GetScrollY();

				ImGui::EndChild();
				ImGui::PopStyleVar(2);

				ImGui::BeginChild("OptionNames");
				scrollx = rescale(scrollx, 0.0f, csMax, 0.0f, columnsScrollM); //No luck using invisible buttons as spacers here, so this fuckery for now, which isn't great either
				ImGui::SetScrollX(scrollx);
				ImGui::EndChild();

				ImGui::BeginChild("ProviderNames");
				ImGui::SetScrollY(scrolly);
				ImGui::EndChild();


			}



		}
		else {
			ImGui::SetNextWindowContentWidth(max<float>(120.0f, audio->modMatrix->consumers.size() * 120.0f));
			float w = ImGui::GetWindowWidth();
			ImGui::BeginChild("ConsumerNames", ImVec2(w - 120.0f - 25.0f, ImGui::GetFontSize() + 14.0f), true, ImGuiWindowFlags_NoScrollbar);
			ImGui::Columns(max<int>(1, audio->modMatrix->consumers.size()), "consumers");

			for (auto& cep : audio->modMatrix->consumers) {
				ImGui::Text(cep->name.c_str()); ImGui::NextColumn();
			}


			auto columnsScrollM = ImGui::GetScrollMaxX();
			ImGui::Columns(1);
			ImGui::EndChild();
			posCache = ImGui::GetCursorPos();
			ImGui::PopStyleVar(2);


			if (audio->modMatrix->providers.size() > 0) {
				float scrollx = 0.0f;
				float scrolly = 0.0f;

				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
				ImGui::BeginChild("ProviderNames", ImVec2(120.0f, 0.0f), true, ImGuiWindowFlags_NoScrollbar);

				for (auto& pep : audio->modMatrix->providers) {
					ImGui::Text(pep->name.c_str());
					ImGui::SameLine();
					ImGui::InvisibleButton("", ImVec2(1.0f, 19.0f));
					ImGui::Separator();
				}

				ImGui::InvisibleButton("", ImVec2(1.0f, 25.0f));
				auto providerLength = ImGui::GetWindowHeight();
				ImGui::EndChild();
				ImGui::PopStyleVar(2);

				posCache.x = posCache.x + 120.0f;
				ImGui::SetCursorPos(posCache);


				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
				ImGui::SetNextWindowContentWidth(max<float>(120.0f, (audio->modMatrix->consumers.size() * 120.0f)));
				ImGui::BeginChild("ConnectionTable", ImVec2(0, providerLength), true, ImGuiWindowFlags_HorizontalScrollbar);


				if (audio->modMatrix->consumers.size() > 0) {
					ImGui::Columns(audio->modMatrix->consumers.size(), "consumers");
				}

				for (auto& pep : audio->modMatrix->providers) {
					for (auto& cep : audio->modMatrix->consumers) {

						const auto k = audio->modMatrix->key(reinterpret_cast<uintptr_t>(pep), reinterpret_cast<uintptr_t>(cep));

						ImGui::PushID(k);

						bool found = false;
						for (auto& c : audio->modMatrix->connectionSet[audio->modMatrix->getCurrentFrame()].connections) {
							if (c.provider == pep && c.consumer == cep) {
								found = true;

								ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.1783f, 0.6f, 0.6f));
								ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.1783f, 0.7f, 0.7f));
								ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.1783f, 0.8f, 0.8f));

								if (ImGui::Button("Set", ImVec2(40, 19))) {
									audio->modMatrix->setConnection(audio->modMatrix->getCurrentFrame(), pep, cep);
								}

								ImGui::SameLine();
								ImGui::PopStyleColor(3);
								ImGui::DragFloat("", &c.targetAttenuation, 0.001f, 0.0f, 1.0f);
							}

						}

						if (found == false) {

							ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.654f, 0.6f, 0.6f));
							ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.654f, 0.7f, 0.7f));
							ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.654f, 0.8f, 0.8f));

							if (ImGui::Button("UnSet", ImVec2(40, 19))) {
								audio->modMatrix->setConnection(audio->modMatrix->getCurrentFrame(), pep, cep);
							}

							ImGui::PopStyleColor(3);

						}


						ImGui::NextColumn();
						ImGui::PopID();

					}

					ImGui::Separator();
				}

				auto csMax = ImGui::GetScrollMaxX();

				scrollx = ImGui::GetScrollX();
				scrolly = ImGui::GetScrollY();

				ImGui::EndChild();
				ImGui::PopStyleVar(2);

				ImGui::BeginChild("ConsumerNames");
				scrollx = rescale(scrollx, 0.0f, csMax, 0.0f, columnsScrollM); //No luck using invisible buttons as spacers here, so this fuckery for now, which isn't great either
				ImGui::SetScrollX(scrollx);
				ImGui::EndChild();

				ImGui::BeginChild("ProviderNames");
				ImGui::SetScrollY(scrolly);
				ImGui::EndChild();
		

			}

		


		}

		ImGui::PopID();
		ImGui::End();

	}

	//
	//
	// Fx
	//
	//

	void fxGUI() {
		ImGui::PushStyleColor(ImGuiCol_TitleBg, (ImVec4)ImColor::HSV(channelHues[activeChannel], 0.5f, 0.5f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_TitleBgActive, (ImVec4)ImColor::HSV(channelHues[activeChannel], 0.5f, 0.5f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, (ImVec4)ImColor::HSV(channelHues[activeChannel], 0.5f, 0.5f, 0.2f));

		ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_FirstUseEver);
		ImGui::Begin("Channel Fx", &showClock);
		ImGui::PushID("fx");

		if (ImGui::Button("Add Filter", ImVec2(80.0,20.0))) {
			audio->channels[activeChannel]->addFx(0);
		}
		ImGui::SameLine();
		if (ImGui::Button("Add Chorus", ImVec2(80.0, 20.0))) {
			audio->channels[activeChannel]->addFx(1);
		}
		ImGui::SameLine();
		if (ImGui::Button("Add Phaser", ImVec2(80.0, 20.0))) {
			audio->channels[activeChannel]->addFx(2);
		}
		ImGui::SameLine();
		if (ImGui::Button("Add Delay", ImVec2(80.0, 20.0))) {
			audio->channels[activeChannel]->addFx(3);
		}
		ImGui::SameLine();
		if (ImGui::Button("Add Reverb", ImVec2(80.0, 20.0))) {
			audio->channels[activeChannel]->addFx(4);
		}
		ImGui::SameLine();
		if (ImGui::Button("Add Granular", ImVec2(80.0, 20.0))) {
			audio->channels[activeChannel]->addFx(5);
		}

		for (auto& fx : audio->channels[activeChannel]->fxChain) {
			if (fx) {
				int tmp = 0;
				const string name = fx->myType();
		
				//if (name.compare("struct BlackStarEffect") == 0) {
				//	const auto bsp = dynamic_pointer_cast<BlackStarEffect, AudioUnit>(fx);
				//
				//	ImGui::PushID("LoadSample");
				//
				//	ImGui::Text("Current Sample: ");
				//	ImGui::SameLine();
				//
				//	if (ImGui::Button(bsp->currentSample().c_str())) {
				//		ImGui::OpenPopup("select_reverb_popup");
				//	}
				//
				//	if (ImGui::BeginPopup("select_reverb_popup")) {
				//
				//		for (int s = 0; s < audio->sampleManager->getSampleList().size(); s++) {
				//			if (ImGui::Selectable(audio->sampleManager->getSampleList()[s].c_str())) {
				//				bsp->loadSample(audio->sampleManager->getSample(s));
 				//				ImGui::CloseCurrentPopup();
				//			}
				//		}
				//
				//		ImGui::EndPopup();
				//	}
				//
				//	ImGui::PopID();
				//
				//}
		
				ImGui::PushID("Options");
				ImGui::PushID(fx->myType());
				for (auto& o : fx->getOptions()) {
					ImGui::PushID(tmp++);
					ImGui::SliderFloat(o.name.c_str(), o.resP, o.min, o.max);
					ImGui::PopID();
				}
				ImGui::PopID();
				ImGui::PopID();
		
				ImGui::PushID("Consumers");
				for (auto& o : fx->getInterface()) {
					ImGui::PushID(tmp++);
					ImGui::SliderFloat(o.name.c_str(), o.resP, o.min, o.max);
					ImGui::PopID();
				}
				ImGui::PopID();
		
			}
		
		
		}

		ImGui::PopID();
		ImGui::End();

		ImGui::PopStyleColor(3);
	}




	//
	//
	// Kit manager
	//
	//
	void kitManagerGUI() {


		ImGui::PushStyleColor(ImGuiCol_TitleBg, (ImVec4)ImColor::HSV(channelHues[activeChannel], 0.5f, 0.5f, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_TitleBgActive, (ImVec4)ImColor::HSV(channelHues[activeChannel], 0.5f, 0.5f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, (ImVec4)ImColor::HSV(channelHues[activeChannel], 0.5f, 0.5f, 0.2f));

		ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_FirstUseEver);
		ImGui::Begin("Kit Editor", &showClock, ImGuiWindowFlags_HorizontalScrollbar);
		ImGui::PushID("kiteditor");

		//Kit Editor
		auto& kit = audio->channels[activeChannel]->local_kit;


		ImGui::Text("Current Kit: ");
		ImGui::SameLine();
		ImGui::Text(to_string(audio->channels[activeChannel]->currentKit).c_str());
		ImGui::SameLine();
		if (ImGui::Button("Save")) {
			audio->channels[activeChannel]->saveKit();
		}
		ImGui::SameLine();

		ImGui::PushID("SaveAs");

		if (ImGui::Button("Save As")) {
			ImGui::OpenPopup("saveas_kit_popup");
		}

		if (ImGui::BeginPopup("saveas_kit_popup")) {

			for (int i = 0; i < audio->kitManager->size(); i++) {
				if (ImGui::Button(to_string(i).c_str(), ImVec2(20.0, 20.0))) {
					audio->channels[activeChannel]->saveKit(i);
					ImGui::CloseCurrentPopup();
				}
				if ((i % 8) < 7) ImGui::SameLine();
			}
			ImGui::EndPopup();
		}

		ImGui::PopID();

		ImGui::NewLine();

		ImGui::PushID("Octave");
		for (int i = 0; i < POLY; i++) {
			ImGui::SameLine();
			ImGui::PushID(i);
			if (ImGui::Button(octaveNames[kit.notes[i].octave()].c_str())) {
				ImGui::PopID(); //This is a touch tortured to get the popup label into the correct scope
				ImGui::OpenPopup("ChangeOctave");
				tmp = i;
			}
			else {
				ImGui::PopID();
			}
		}


		if (ImGui::BeginPopup("ChangeOctave"))
		{
			for (int s = 0; s < octaveNames.size(); s++) {
				ImGui::PushID(s);
				if (ImGui::Selectable(octaveNames[s].c_str())) {
					kit.notes[tmp].octave(s);
					ImGui::CloseCurrentPopup();
				}
				ImGui::PopID();
			}
			ImGui::EndPopup();
		}

		ImGui::PopID();

		ImGui::NewLine();

		ImGui::PushID("Note");
		for (int i = 0; i < POLY; i++) {
			ImGui::SameLine();
			ImGui::PushID(i);
			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(noteHues[kit.notes[i].relative()], 0.5f, 0.5f));
			//ImGui::PushStyleColor(ImGuiCol_Button, noteColours[kit.notes[i].relative()]);
			if (ImGui::Button(noteNames[kit.notes[i].relative()].c_str())) {
				ImGui::PopID(); //This is a touch tortured to get the popup label into the correct scope
				ImGui::OpenPopup("ChangeNote");
				tmp = i;
			}
			else {
				ImGui::PopID();
			}
			ImGui::PopStyleColor();
		}


		if (ImGui::BeginPopup("ChangeNote"))
		{
			for (int s = 0; s < noteNames.size(); s++) {
				ImGui::PushID(s);
				if (ImGui::Selectable(noteNames[s].c_str())) {
					kit.notes[tmp].relative(s);
					ImGui::CloseCurrentPopup();
				}
				ImGui::PopID();
			}
			ImGui::EndPopup();
		}

		ImGui::PopID();

		ImGui::NewLine();

		if (ImGui::Button("Set Scale")) {
			ImGui::OpenPopup("set_scale");
		}

		if (ImGui::BeginPopup("set_scale"))
		{

			//const vector<string> scaleNames = { "Major", "Natural Minor", "Harmonic Minor", "Melodic Minor", "Ionian", "Dorian", "Phrygian", "Lydian", "Myxolydian", "Aeolian", "Locrian" };
			//int currentScaleType = 0;
			//const vector<string> chordTypeNames = { "Diminished", "Half Dimished", "Minor", "Minor-Major", "Dominant", "Major", "Augmented", "Augmented Major", "Sus2", "Sus4" };
			//int currentChordType = 0;
			static int tmpOctave = 5;
			static int tmpNote = 0;
			static int tmpScale = 0;
			static int tmpstart = 0;
			static int tmpend = POLY - 1;

			ImGui::DragInt("Octave", &tmpOctave, 1.0f, 1, 8);
			ImGui::Combo2("Root Note", &tmpNote, noteNames);
			ImGui::Combo2("Scale Type", &tmpScale, scaleNames);
			ImGui::DragInt("Start", &tmpstart, 1.0f, 1, 8);
			ImGui::DragInt("End", &tmpend, 1.0f, 1, 8);

			if (ImGui::Button("Okay")) {
				kit.write_scale(12 * tmpOctave + tmpNote, static_cast<ChromaticScale::ScaleTypes>(tmpScale), tmpstart, tmpend);
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button("Set Chord")) {
			ImGui::OpenPopup("set_chord");
		}

		if (ImGui::BeginPopup("set_chord"))
		{

			static int tmpOctave = 5;
			static int tmpNote = 0;
			static int tmpChord = 0;
			static int tmpstart = 0;
			static int tmpend = POLY - 1;

			ImGui::DragInt("Octave", &tmpOctave, 1.0f, 1, 8);
			ImGui::Combo2("Root Note", &tmpNote, noteNames);
			ImGui::Combo2("Chord Type", &tmpChord, chordTypeNames);
			ImGui::DragInt("Start", &tmpstart, 1.0f, 1, POLY-1);
			ImGui::DragInt("End", &tmpend, 1.0f, 1, POLY - 1);



			if (ImGui::Button("Okay")) {
				kit.write_chord(12 * tmpOctave + tmpNote, static_cast<ChromaticSeventh::Types>(tmpChord), tmpstart, tmpend);
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::NewLine();

		bool selectedKitNumber = false;

		ImGui::PushID("Sample Picker");

		for (int i = 0; i < POLY; i++) {
			//Figure out how to get this here
			//const float pbratio = pads[i]->playbackCompletePercent();
			//ImGui::PushStyleColor(ImGuiCol_PlotLines, (ImVec4)ImColor::HSV(i / 12.0f, pbratio, pbratio));
			ImGui::PushID(i);
			if (kit.current_samples[i]) {
				ImGui::PushStyleColor(ImGuiCol_PlotLines, (ImVec4)ImColor::HSV(noteHues[kit.notes[i].relative()], 0.5f, 0.5f));
				ImGui::PlotLines("", kit.current_samples[i]->getData()->data(), kit.current_samples[i]->getData()->size(), 0, to_string(i).c_str(), -1.0f, 1.0f, ImVec2(50, 50));
				if (ImGui::IsItemClicked()) {
					ImGui::PopID();
					ImGui::OpenPopup("ChangeSample");
					tmp = i;
				}
				else {
					ImGui::PopID();
				}
				ImGui::PopStyleColor();
			}
			else {
				const float dummy[1] = { 0.0f };

				ImGui::PlotLines("", dummy, 1, 0, "", -1.0f, 1.0f, ImVec2(50, 50));
				if (ImGui::IsItemClicked()) {
					ImGui::PopID();
					ImGui::OpenPopup("ChangeSample");
					tmp = i;
				}
				else {
					ImGui::PopID();
				}
			}

			if ((i % 4) < 3) ImGui::SameLine();

			//array< shared_ptr<Sample>, 8 > currentSamples;
		}

		if (ImGui::BeginPopup("ChangeSample"))
		{
			for (int s = 0; s < audio->sampleManager->getSampleList().size(); s++) {
				//ImGui::PushID(id+s+1);
				if (ImGui::Selectable(audio->sampleManager->getSampleList()[s].c_str())) {
					kit.current_samples[tmp] = audio->sampleManager->getSample(s);
					ImGui::CloseCurrentPopup();
				}
				//ImGui::PopID();
			}
			ImGui::EndPopup();
		}

		ImGui::PopID();



		ImGui::Separator();
		ImGui::Text("Rules:");
		ImGui::PushID("KitRulesEditing");
		if (ImGui::Button("Disable Rules")) {
			for (auto& r : audio->channels[activeChannel]->local_kit.rules) {
				r.run = false;
				r.event_counter = 0;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Add Rule")) {
			audio->channels[activeChannel]->local_kit.rules.push_back(Rule<Kit::Events, Kit::Actions>());
		}

		//Yep columns really do need some love
		ImGui::BeginChild("KitRules", ImVec2(audio->channels[activeChannel]->local_kit.rules.size() * 292, 212), false, ImGuiWindowFlags_HorizontalScrollbar);
		ImGui::PushID("KRules");

		ImGui::Columns(audio->channels[activeChannel]->local_kit.rules.size() + 1);
		int tmp = 0;
		for (auto& r : audio->channels[activeChannel]->local_kit.rules) {
			ImGui::PushID(tmp++);
			ImGui::Checkbox("Run", &r.run);

			current_kit_event = r.event;
			if (ImGui::Combo2("Type", reinterpret_cast<int*>(&current_kit_event), kit_event_names)) {
				r.event = current_kit_event;
			}

			ImGui::SliderInt("Event Src", &r.event_src, 0, POLY-1);

			ImGui::DragInt("Action Count", &r.action_threashold);

			current_kit_action = r.action;
			if (ImGui::Combo2("Action", reinterpret_cast<int*>(&current_kit_action), kit_action_names)) {
				r.action = current_kit_action;
			}

			//I should look into moving these limits and such into rules or something
			//it's probably not great this is the only place it's written out
			switch (current_kit_action) {
			case Kit::Actions::LOAD:
				ImGui::SliderInt("Action Src", &r.action_src, 0, 63);
				break;
			case Kit::Actions::SWAP_NOTES:
				ImGui::SliderInt("Src Note", &r.action_src, 0, POLY - 1);
				ImGui::SliderInt("Dst Note", &r.action_dst, 0, POLY - 1);
				break;
			case Kit::Actions::SWAP_SAMPLES:
				ImGui::SliderInt("Src Sample", &r.action_src, 0, POLY - 1);
				ImGui::SliderInt("Dst Sample", &r.action_dst, 0, POLY - 1);
				break;
			case Kit::Actions::ROTATE_NOTES_FWD:
				ImGui::SliderInt("Rotate From", &r.action_src, 0, POLY - 1); //Hmmm POLY-1 or POLY here... hmmm
				ImGui::SliderInt("Rotate To", &r.action_dst, 0, POLY - 1);
				break;
			case Kit::Actions::ROTATE_NOTES_BCK:
				ImGui::SliderInt("Rotate From", &r.action_src, 0, POLY - 1);
				ImGui::SliderInt("Rotate To", &r.action_dst, 0, POLY - 1);
				break;
			case Kit::Actions::ROTATE_SAMPLES_FWD:
				ImGui::SliderInt("Rotate From", &r.action_src, 0, POLY - 1);
				ImGui::SliderInt("Rotate To", &r.action_dst, 0, POLY - 1);
				break;
			case Kit::Actions::ROTATE_SAMPLES_BCK:
				ImGui::SliderInt("Rotate From", &r.action_src, 0, POLY - 1);
				ImGui::SliderInt("Rotate To", &r.action_dst, 0, POLY - 1);
				break;
			case Kit::Actions::TURING_NOTES_KIT:
			case Kit::Actions::TRANSPOSE_NOTES:
				ImGui::SliderInt("Note +/-", &r.action_src, -24, 24); // user convience only, currently 108... thinking min / max could be user selectable
				ImGui::SliderInt("Note Start", &r.action_range_start, 0, POLY - 1);
				ImGui::SliderInt("Note End", &r.action_range_end, 0, POLY - 1);
				break;
			case Kit::Actions::TRANSPOSE_OCTAVES:
				ImGui::SliderInt("Octave +/-", &r.action_src, -8, 8);
				ImGui::SliderInt("Note Start", &r.action_range_start, 0, POLY - 1);
				ImGui::SliderInt("Note End", &r.action_range_end, 0, POLY - 1);
				break;
			case Kit::Actions::WRITE_CHORD:
				{
					//write_chord(Note(rule.action_src), static_cast<ChromaticSeventh::Types>(rule.action_dst), rule.action_range_start, rule.action_range_end);
					Note display;
					display.n = r.action_src;
					int tmp_octave = display.octave();

					ImGui::SliderInt("Action Src", &r.action_src, 0, POLY - 1);
					ImGui::Text("Root:");
					ImGui::Text("Octave:");
					ImGui::SameLine();
					ImGui::DragInt("Octave", &tmp_octave, 1.0f, 1, 8);
					ImGui::Text("Note:");
					ImGui::SameLine();
					if (ImGui::Combo2("Root Note", &display.n, noteNames)) {
						r.action_src = display.n * tmp_octave * 12;
					}


					ImGui::Combo2("Chord Type", &r.action_dst, chordTypeNames);
					ImGui::SliderInt("Range Start", &r.action_range_start, 0, POLY - 1);
					ImGui::SliderInt("Range End", &r.action_range_end, 0, POLY - 1);
				}
				
				break;
			case Kit::Actions::ENABLE_RULE:
				ImGui::SliderInt("Action Src", &r.action_src, 0, POLY - 1);
				ImGui::SliderInt("Action Dst", &r.action_dst, 0, POLY - 1);
				ImGui::SliderInt("Range Start", &r.action_range_start, 0, POLY - 1);
				ImGui::SliderInt("Range End", &r.action_range_end, 0, POLY - 1);
				break;
			case Kit::Actions::DISABLE_RULE:
			default:
				ImGui::SliderInt("Action Src", &r.action_src, 0, POLY - 1);
				ImGui::SliderInt("Action Dst", &r.action_dst, 0, POLY - 1);
				ImGui::SliderInt("Range Start", &r.action_range_start, 0, POLY - 1);
				ImGui::SliderInt("Range End", &r.action_range_end, 0, POLY - 1);
				break;
			}


			ImGui::PopID();
			ImGui::NextColumn();
		}

		ImGui::Columns(1);
		ImGui::PopID();
		ImGui::EndChild();

		ImGui::PopID();
		ImGui::End();

		ImGui::PopStyleColor(3);

	}









	void sfxTestGUI() {
		ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_FirstUseEver);
		ImGui::Begin("Sfx Player", &showMixer, ImGuiWindowFlags_HorizontalScrollbar);

		static int priority = 0;
		static float pos = 0.5f;
		static float dir = 1.0f;
		static float vel = 0.0f;
		static float level = 1.0f;

		if (audio->channels[7]->audioEngine) {
			const string running_type = audio->channels[7]->audioEngine->myType();

			if (running_type.compare("struct SfxPlayer") == 0) {

				const auto sfx = dynamic_pointer_cast<SfxPlayer, EngineUnit>(audio->channels[7]->audioEngine);

				//play_sfx(const int priority, const string & sfx_name_, const float pos_ = 0.5f,
				//	const float direction_ = 0.0f, const float veclocity_ = 0.0f, const float level_ = 1.0f, const bool loop_ = false)

				ImGui::DragInt("Priority", &priority, 1.0f, 1, 255);
				ImGui::SliderFloat("Pos", &pos, 0.0f, 1.0f);
				ImGui::SliderFloat("Dir", &dir,-1.0f, 1.0f);
				ImGui::SliderFloat("Vel", &vel, 0.0f, 1000.0f);
				ImGui::SliderFloat("Level", &level, 0.0f, 1.0f);

				if (ImGui::Button("Play Sound: Numbers Loop")) {
					sfx->play_sfx(priority, "Samples/Long/E03a-2006-05-09-1004utc-by_tING.wav", pos, dir, vel, level, true);
				}

				if (ImGui::Button("Play Sound: Boom")) {
					sfx->play_sfx(priority, "Samples/sfx/boom.wav", pos, dir, vel, level, false);
				}

				ImGui::PlotLines("Debug Scope", &audio->channels[7]->audioEngine->shittyscope[0], static_cast<int>(scopeScale * audio->channels[7]->audioEngine->shittyscope.size()), 0, "Sample", -1.0f, 1.0f, ImVec2(200, 100));
				ImGui::SliderFloat("Scope Scale", &scopeScale, 0.1, 1.0f);
			}
					   		
		}

		ImGui::End();

	}






















	//
	//
	// Run
	//
	//

	bool run() {

		if (!glfwInit()) {
			cout << "Failed to initialize GLFW" << endl;
		}

		//GLFW config
		glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing

		const char* glsl_version = "#version 430 core";
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

		window = glfwCreateWindow(win_w, win_h, "Series", NULL, NULL);
		if (window == NULL) {
			cout << "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible." << endl;
			glfwTerminate();
		}

		glfwMakeContextCurrent(window);
		glfwSwapInterval(1); //vsync on

		glewExperimental = true;
		if (glewInit() != GLEW_OK) {
			cout << "Failed to initialize GLEW" << endl;
		}

		glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
		glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);



		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io; //wtftf??
		io.ConfigWindowsMoveFromTitleBarOnly = true;

		//ImGui::StyleColorsDark();
		ImGui::StyleColorsClassic();

		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init(glsl_version);

		io.Fonts->AddFontDefault();

		while (!glfwWindowShouldClose(window)) {

			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			if (ImGui::IsKeyPressed(290, false))  //F1
				activeChannel = 0;
			if (ImGui::IsKeyPressed(291, false))  //F2
				activeChannel = 1;	
			if (ImGui::IsKeyPressed(292, false))  //F3
				activeChannel = 2;	
			if (ImGui::IsKeyPressed(293, false))  //F4
				activeChannel = 3;	
			if (ImGui::IsKeyPressed(294, false))  //F5
				activeChannel = 4;	
			if (ImGui::IsKeyPressed(295, false))  //F6
				activeChannel = 5;	
			if (ImGui::IsKeyPressed(296, false))  //F7
				activeChannel = 6;	
			if (ImGui::IsKeyPressed(297, false))  //F8
				activeChannel = 7;


			if (show_save_window) {
				ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_FirstUseEver);
				ImGui::Begin("Savefile Options", &show_save_window);
				ImGui::Separator();
				if (ImGui::Button("Dump Json")) {
					audio->dumpConfig();
				}

				ImGui::InputText("Save File", audioSaveFile, ((int)(sizeof(audioSaveFile) / sizeof(*audioSaveFile))));
				ImGui::SameLine();
				if (ImGui::Button("Save Json")) {
					string filename = audioSaveFile;
					audio->saveConfig(filename);
				}
				
				ImGui::InputText("Load File", audioLoadFile, ((int)(sizeof(audioLoadFile) / sizeof(*audioLoadFile))));
				ImGui::SameLine();
				if (ImGui::Button("Load Json")) {
					string filename = audioLoadFile;
					audio->stopSound();
					audio->loadConfig(filename);
					audio->startSound();
				}

				ImGui::End();
			}

			//
			//Need kit editor with nice music options
			//
			//Kit import export
			//Measure import export
			//
			// check frame modulation of connection attenuations
			//
			//Mod maxtrix changes?
			//  allow cross channel connections in gui
			//
			// load json doesn't clear channels properly *should be fixed
			//     fx chains still there
			//     local measures still there
			//     local kit still there
			//
			//change rotate labels / directions for least surprise
			//
			//pos / neg limits on drag bars not working * read not set
			//
			//Mixed up pan / detune in synth voice * should be fixed
			//
			//
			//Features to add (no particular order)
			// better sample back end + packager
			//   pcm style sample playback: play till looping section, then loop looping section
			//   probably need to be a little more fancy, eg loop looping secion and envelope in attack transient section
			// Sfx Playback w priority and position (maybe velocity)
			// Midi file support / import / gui
			// dc vca in modulation like sandh
			// global (eg non-poly) modulation sources other then mod_matrix's const
			//


			clockGUI();
			mixerGUI();
			channelEngineGUI();
			channelMeasureGUI();
			modulationGUI();
			modMatrixGUI();
			fxGUI();
			kitManagerGUI();
			sfxTestGUI();


			//ImGui::ShowDemoWindow();

			ImGui::Render();
			int display_w, display_h;
			glfwGetFramebufferSize(window, &display_w, &display_h);
			glViewport(0, 0, display_w, display_h);
			glClear(GL_COLOR_BUFFER_BIT);
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			glfwSwapBuffers(window);

			glfwPollEvents();

		}

		return true;
	}



	~SeriesGfx() {
		audio->stopSound();

		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		glfwDestroyWindow(window);
		glfwTerminate();

		window = nullptr;
		audio = nullptr;
	}

};