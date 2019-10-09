#pragma once

#include "imgui.h"
#include <vector>

//Libsndfile
#include <sndfile.hh>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <list>
#include <memory>
#include <limits>

#include "BaseObjects.h"
#include "Trigger.h"
#include "Effect.h"



using namespace std;


//Events trigger:
//12 pads
//Each pad has a sample src + effect chain
//Effect chain stuff
//filter
//delay
//reverb
//
//12 pads summed output by global effect / mixer
//sound out



std::vector<std::string> sampleList = {
	"Samples/Breaks/tighten_up.wav",
	"Samples/Breaks/Tighten_Up1.wav",
	"Samples/Breaks/Tighten_Up2.wav",
	"Samples/ClosedHats/LM-1/HiHat10.wav",
	"Samples/Kick/BigOldKick.wav",
	"Samples/Kick/BigOldKickLong.wav",
	"Samples/Percussion/DMX/23_RIMSHOT.wav",
	"Samples/Percussion/EstecDrums/estecdrumspk.aif",
	"Samples/Percussion/EstecDrums/estecdrumspk10.aif",
	"Samples/Percussion/EstecDrums/estecdrumspk11.aif",
	"Samples/Percussion/EstecDrums/estecdrumspk12.aif",
	"Samples/Percussion/EstecDrums/estecdrumspk13.aif",
	"Samples/Percussion/EstecDrums/estecdrumspk14.aif",
	"Samples/Percussion/EstecDrums/estecdrumspk15.aif",
	"Samples/Percussion/EstecDrums/estecdrumspk16.aif",
	"Samples/Percussion/EstecDrums/estecdrumspk2.aif",
	"Samples/Percussion/EstecDrums/estecdrumspk3.aif",
	"Samples/Percussion/EstecDrums/estecdrumspk4.aif",
	"Samples/Percussion/EstecDrums/estecdrumspk5.aif",
	"Samples/Percussion/EstecDrums/estecdrumspk6.aif",
	"Samples/Percussion/EstecDrums/estecdrumspk7.aif",
	"Samples/Percussion/EstecDrums/estecdrumspk8.aif",
	"Samples/Percussion/EstecDrums/estecdrumspk9.aif"
};


std::vector<std::string> effectList = {
	"Vca",
	"Filter",
	"Delay",
	"Distortion",
	"Shape"
};

struct Pad : AudioUnit {
protected:
	Trigger trigger;

	vector<float> bufferL;
	vector<float> bufferR;
	list< unique_ptr<AudioUnit> > fxChain;
	int selectedSample = -1;
	int selectedEffect = -1;
	size_t sampleIdx = numeric_limits<size_t>::max(); //This will behave a little oddly if a sample this large is ever loaded har har
	int soundStart = 0; // 0 - 1
	int soundEnd = 0; // 0 - 1
	float variablePos = 0.0f;
	float variableMix = 0.0f;
	float variableStepScale = 0.0f; //0.03125 - 32

	float gain = 1.0f;
	float pan = 0.5f;
	float triggerMod = 0.0f;

	bool loop = false;
	bool running = false;

private:

	inline float panL(const float pan_) {
		return cos(M_PI * (pan_) / 2.0f);
	}

	inline float panR(const float pan_) {
		return sin(M_PI * (pan_) / 2.0f);
	}

	inline float clamp(const float x_, const float min_, const float max_) {
		return fmaxf(fminf(x_, fmaxf(min_, max_)), fminf(min_, max_));
	}


public:
	bool showWindow = false;

	Pad(const string name_, const float sampleRate_, Input* userParams_, shared_ptr<ModMatrix> matrix_, shared_ptr<NoteMatrix> noteMatrix_) {
		sampleRate = sampleRate_;
		name = name_;
		userParams = userParams_;
		matrix = matrix_;
		noteMatrix = noteMatrix_;
		running = true;

		bufferL.resize(1, 0.0f);
		bufferR.resize(1, 0.0f);


		interfaceList = {
			{ true,	name + " Trig", 0.0f, 1.0f, &triggerMod },
			{ true, name + " Pan", 0.0f, 1.0f, &pan }
		};

	}

	const std::vector<float>* currentSampleData() {
		return &bufferL;
	}

	const float playbackCompletePercent() {
		if (bufferL.size() > 0) {
			return clamp(static_cast<float>(sampleIdx) / static_cast<float>(bufferL.size()), 0.0f, 1.0f);
		}
		return 0.0f;
	}

	//hardcoded from effectList above
	void addEffect(const int idx_) {
		switch (idx_) {
		case 0:
			fxChain.push_back(make_unique<Vca>(name + "Vca", sampleRate,  userParams, matrix, noteMatrix));
			break;
		case 1:
			fxChain.push_back(make_unique<FilterEffect>(name + "Filter", sampleRate, userParams, matrix, noteMatrix));
			break;
		case 2:
			fxChain.push_back(make_unique<DelayEffect>(name + "Delay", sampleRate, userParams, matrix, noteMatrix));
			break;
		case 3:
			fxChain.push_back(make_unique<DistortionEffect>(name + "Dist", sampleRate, userParams, matrix, noteMatrix));
			break;
		case 4:
			fxChain.push_back(make_unique<ShapeEffect>(name + "Shape", sampleRate, userParams, matrix, noteMatrix));
			break;
		default:
			break;
		}
	}
	
	void loadSample(const int sampleIdx_) {
		SndfileHandle file;
		std::vector<float> tmp;

		sampleIdx = numeric_limits<size_t>::max();
		
		file = SndfileHandle(sampleList[sampleIdx_].c_str());

		std::cout << "loadSample - " << sampleList[sampleIdx_] << std::endl;

		if (!file.error()) {
			std::cout << " channels " << file.channels() << " samplingRate " << file.samplerate() << " Format " << std::hex << file.format() << std::dec << std::endl;
			std::cout << " samples " << file.frames() << std::endl;

			bufferL.resize(file.frames(), 0.0f);
			bufferR.resize(file.frames(), 0.0f);

			fill(bufferL.begin(), bufferL.end(), 0.0f);
			fill(bufferR.begin(), bufferR.end(), 0.0f);


			sampleIdx = file.frames();
			tmp.resize(file.frames() * file.channels());

			file.readf(tmp.data(), file.frames());

			//Left channel only
			if (file.channels() == 2) {
				int tmpIdx = 0;
				for (int c = 0; c < file.frames(); c++) {
					bufferL[c] = tmp[tmpIdx];
					bufferR[c] = tmp[tmpIdx+1];
					tmpIdx += 2;
				}
			}
			else {
				for (int c = 0; c < tmp.size(); c++) {
					bufferL[c] = tmp[c];
					bufferR[c] = tmp[c];
				}
			}

			selectedSample = sampleIdx_;
		}
		else {

			std::cout << "sndfile Error " << file.strError() << std::endl;

		}

	}

	const std::string getName() {
		return name;
	}

	void triggerSample() {
		//std::cout << name << " Triggered" << std::endl;
		sampleIdx = 0;
	}

	//L+R are the same length, call after getL / getR
	void step() {
		if (trigger.process(triggerMod)) {
			sampleIdx = 0;
		}

		if (sampleIdx < bufferL.size()) {
			sampleIdx++;
		}
	}


	virtual void processAudio(pair<float, float> &in_) {

		pair<float, float> sample = make_pair(0.0f, 0.0f);

		if (trigger.process(triggerMod)) {
				sampleIdx = 0;
		}

		if(sampleIdx < bufferL.size()) {
			sample = make_pair(bufferL[sampleIdx] * gain *panL(pan), bufferR[sampleIdx] * gain * panR(pan));
			sampleIdx++;
		}	else {
			if (loop) {
				sampleIdx = 0;
				sample = make_pair(bufferL[sampleIdx] * gain * panL(pan), bufferR[sampleIdx] * gain * panR(pan));
				sampleIdx++;
			}
		}

		for (auto &fx : fxChain) {
			fx->processAudio(sample);
		}

		in_ = sample;

	}


	void display() {
		ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_FirstUseEver);
		ImGui::Begin(name.c_str(), &showWindow);
		ImGui::PushID(this);

		int item_clicked = -1;
		ImGui::PushID("SampleList");
		if (ImGui::ListBox("Samples", &selectedSample, sampleList)) {
			loadSample(selectedSample);
		}
		ImGui::PopID();

		ImGui::PushID("EffectsList");
		if (ImGui::ListBox("Fx", &selectedEffect, effectList)) {
			addEffect(selectedEffect);
		}
		ImGui::PopID();
		
		ImGui::Text("Register Trigger");
		mplusButton(0);

		ImGui::DragFloat("Pan", &pan, 0.001f, 0.0f, 1.0f, "%.03f");
		mplusButton(1);

		ImGui::DragFloat("Gain", &gain, 0.001f, 0.0f, 1.0f, "%.03f");

		ImGui::PlotLines("Current Sample", &bufferL[0], bufferL.size(), 0, "Sample", -1.0f, 1.0f, ImVec2(100, 50));
		if (ImGui::Button("Loop")) {
			loop = !loop;
		}
		if (loop) {
			ImGui::SameLine(); ImGui::Text("looping");
		}

		ImGui::PushID("t");
		if (ImGui::Button("Trigger")) {
			triggerSample();
		}
		ImGui::PopID();

		for (auto &fx : fxChain) {
			fx->display();
		}

		ImGui::PopID();
		ImGui::End();

//		if (item_clicked >= 0) {
//			loadSample(item_clicked);
//		}

	}


	json toConfig() {
		json config = ModuleBase::toConfig();

		config["selectedSample"] = selectedSample;
		config["gain"] = gain;
		config["pan"] = pan;
		config["loop"] = loop;

		for (auto& fx : fxChain) {
			config["fxChain"].push_back(fx->toConfig());
		}

		return config;
	}


	void loadConfig(const json cfg) {

		ModuleBase::loadConfig(cfg);

		if (cfg.count("selectedSample")) {
			selectedSample = cfg["selectedSample"].get<int>();
			if (selectedSample >= 0) {
				loadSample(selectedSample);
			}
			
		}

		if (cfg.count("gain"))
			gain = cfg["gain"].get<float>();

		if (cfg.count("pan"))
			pan = cfg["pan"].get<float>();

		if (cfg.count("loop"))
			loop = static_cast<bool>(cfg["loop"].get<int>());

		if (cfg.count("fxChain")) {
			for (auto& fxj : cfg["fxChain"]) {

				if (fxj.count("type")) {
					if (fxj["type"].get<string>().compare("struct Vca") == 0) {
						fxChain.push_back(make_unique<Vca>(name + " Vca", sampleRate, userParams, matrix, noteMatrix));
						fxChain.back()->loadConfig(fxj);
					}
					
					if (fxj["type"].get<string>().compare("struct filterEffect") == 0) {
						fxChain.push_back(make_unique<FilterEffect>(name + " Filter", sampleRate, userParams, matrix, noteMatrix));
						fxChain.back()->loadConfig(fxj);
					}

					if (fxj["type"].get<string>().compare("struct DelayEffect") == 0) {
						fxChain.push_back(make_unique<DelayEffect>(name + " Delay", sampleRate, userParams, matrix, noteMatrix));
						fxChain.back()->loadConfig(fxj);
					}

					if (fxj["type"].get<string>().compare("struct DistortionEffect") == 0) {
						fxChain.push_back(make_unique<DistortionEffect>(name + " Dist", sampleRate, userParams, matrix, noteMatrix));
						fxChain.back()->loadConfig(fxj);
					}

					if (fxj["type"].get<string>().compare("struct ShapeEffect") == 0) {
						fxChain.push_back(make_unique<ShapeEffect>(name + " Shape", sampleRate, userParams, matrix, noteMatrix));
						fxChain.back()->loadConfig(fxj);
					}

				}

			}
		}




	}






};







struct PadBlock : AudioUnit {
private:

	vector< unique_ptr<Pad> > pads;
	vector< unique_ptr<Trigger> > triggers;

public:
	bool showWindow = true;
	

	
	PadBlock(const string name_, const float sampleRate_, Input* userParams_, shared_ptr<ModMatrix> matrix_, shared_ptr<NoteMatrix> noteMatrix_) {
		name = name_;
		sampleRate = sampleRate_;
		userParams = userParams_;
		matrix = matrix_;
		noteMatrix = noteMatrix_;

		pads.resize(9);
		for (int i = 0; i < 9; i++) {
			std::ostringstream name;
			name << "Pad" << i;
			pads[i] = make_unique<Pad>(name.str(), sampleRate_, userParams, matrix, noteMatrix);
			//pads[i]->addEffect(0); //Make sure each has a vca at the top of it's chain
		}
		triggers.resize(9);
		for (auto& t : triggers) {
			t = make_unique<Trigger>();
		}


	}

	void triggerPad(const int pad_) {
		pads[pad_]->triggerSample();
	}


	virtual void processAudio(pair<float, float> &in_) {
		float L = 0.0f;
		float R = 0.0f;


		for (auto &p : pads) {
			pair<float, float> s = make_pair(0.0f, 0.0f);
			p->processAudio(s);
			L += get<0>(s);
			R += get<1>(s);
		}

		in_.first = L;
		in_.second = R;

	}




	void display() {
		//for each pads call display if turned on

		ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_FirstUseEver);
		ImGui::Begin("Pad Grid", &showWindow);
		ImGui::PushID(this);

		for (int i = 0; i < 9; i++) {
			ImGui::PushID(i);
			const float pbratio = pads[i]->playbackCompletePercent();
			ImGui::PushStyleColor(ImGuiCol_PlotLines, (ImVec4)ImColor::HSV(i / 12.0f, pbratio, pbratio));
			ImGui::PlotLines("", pads[i]->currentSampleData()->data(), pads[i]->currentSampleData()->size(), 0, pads[i]->getName().c_str(), -1.0f, 1.0f, ImVec2(50, 50));
			if (ImGui::IsItemClicked()) {
				pads[i]->showWindow = true;
			}

			ImGui::PopStyleColor();

			if ((i % 3) < 2) ImGui::SameLine();
			ImGui::PopID();
		}
		ImGui::PopID();

		ImGui::End();

		for (auto &p : pads) {
			if (p->showWindow) {
				p->display();
			}
		}

	}
	   	  
	json toConfig() {
		json config = ModuleBase::toConfig();

		for (auto& p : pads) {
			config["pads"].push_back(p->toConfig());
		}

		return config;
	}


	void loadConfig(const json cfg) {

		ModuleBase::loadConfig(cfg);

		int pIdx = 0;
		if (cfg.count("pads")) {
			for (auto& pj : cfg["pads"]) {
				pads[pIdx]->loadConfig(pj);
				pIdx++;
			}
		}

	}



};



























































































































