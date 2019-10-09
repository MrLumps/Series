#pragma once


#include <math.h>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <list>
#include <thread>
#include <future>
#include <iterator>

#include "imgui.h"
#include "BaseObjects.h"
#include "ModMatrix.h"
#include "Filter.h"

using namespace std;


struct StereoBiQuad {
	array<double, 3> a = { 0.0, 0.0, 0.0 };
	array<double, 3> b = { 0.0, 0.0, 0.0 };
	//x1, x2, y1, y2
	array<double, 4> left = { 0.0, 0.0, 0.0, 0.0 };
	array<double, 4> right = { 0.0, 0.0, 0.0, 0.0 };

	double Q;  // (>0) - 1
	float gain; //db +- 3 or more
	double sampleRate;



	void init(const float sampleRate_) {
		sampleRate = sampleRate_;
		fill(a.begin(), a.end(), 0.0);
		fill(b.begin(), b.end(), 0.0);
		fill(left.begin(), left.end(), 0.0);
		fill(right.begin(), right.end(), 0.0);
		Q = 1.0;
		gain = 0.0f;
	}


	void setPeakingEQ(const float hz_) {
		const double o = 2.0 * M_PI * hz_ / sampleRate;
		const double A = sqrt(pow(10.0, (static_cast<double>(gain)/40.0)));
		const double alpha = sin(o) / (2.0f * Q);

		b[0] = 1.0f + alpha * A;
		b[1] = -2.0f * cos(o);
		b[2] = 1.0f - alpha * A;

		a[0] = 1.0f + alpha / A;
		a[1] = b[1];
		a[2] = 1.0f - alpha / A;

		a[1] /= a[0];
		a[2] /= a[0];
		b[0] /= a[0];
		b[1] /= a[0];
		b[2] /= a[0];
	}

	//Q = shelf slope
	void setLowShelf(const float hz_) {
		const double o = 2.0 * M_PI * hz_ / sampleRate;
		const double A = sqrt(pow(10.0, (static_cast<double>(gain) / 40.0)));
		//sin(w0)/2 * sqrt( (A + 1/A)*(1/S - 1) + 2 )
		const double alpha = sin(o) / 2.0f * sqrt((A + 1.0 / A) * (1.0 / Q - 1.0) + 2.0);

		const double cosO = cos(o);
		const double sqrA = 2.0 * sqrt(A) * alpha;

		b[0] =       A * ((A + 1.0) - (A - 1.0) * cosO + sqrA);
		b[1] = 2.0 * A * ((A - 1.0) - (A + 1.0) * cosO);
		b[2] =       A * ((A + 1.0) - (A - 1.0) * cosO - sqrA);
		a[0] =            (A + 1.0) + (A - 1.0) * cosO + sqrA;
		a[1] =-2.0 *     ((A - 1.0) + (A + 1.0) * cosO);
		a[2] =            (A + 1.0) + (A - 1.0) * cosO - sqrA;

		a[1] /= a[0];
		a[2] /= a[0];
		b[0] /= a[0];
		b[1] /= a[0];
		b[2] /= a[0];
	}

	//Q = shelf slope
	void setHighShelf(const float hz_) {
		const double o = 2.0 * M_PI * hz_ / sampleRate;
		const double A = sqrt(pow(10.0, (static_cast<double>(gain) / 40.0)));
		//sin(w0)/2 * sqrt( (A + 1/A)*(1/S - 1) + 2 )
		const double alpha = sin(o) / 2.0f * sqrt((A + 1.0 / A) * (1.0 / Q - 1.0) + 2.0);

		const double cosO = cos(o);
		const double sqrA = 2.0 * sqrt(A) * alpha;

		b[0] =       A * ((A + 1.0) - (A - 1.0) * cosO + sqrA);
		b[1] =-2.0 * A * ((A - 1.0) - (A + 1.0) * cosO);
		b[2] =       A * ((A + 1.0) - (A - 1.0) * cosO - sqrA);
		a[0] =            (A + 1.0) + (A - 1.0) * cosO + sqrA;
		a[1] = 2.0 *     ((A - 1.0) + (A + 1.0) * cosO);
		a[2] =            (A + 1.0) + (A - 1.0) * cosO - sqrA;

		a[1] /= a[0];
		a[2] /= a[0];
		b[0] /= a[0];
		b[1] /= a[0];
		b[2] /= a[0];
	}


	pair<float, float> process(const pair<float, float> in_) {
		const double l = b[0] * static_cast<double>(in_.first) + b[1] * left[0] + b[2] * left[1] - a[1] * left[2] - a[2] * left[3];
		const double r = b[0] * static_cast<double>(in_.second) + b[1] * left[0] + b[2] * left[1] - a[1] * left[2] - a[2] * left[3];

		left[1] = left[0];
		left[0] = static_cast<double>(in_.first);
		left[3] = left[2];
		left[2] = l;

		right[1] = right[0];
		right[0] = static_cast<double>(in_.second);
		right[3] = right[2];
		right[2] = r;

		return make_pair(static_cast<float>(l), static_cast<float>(r));
	}


};



struct SubMixer : MixerUnit {

	struct ChannelEQ {
		float level = 0.0;

		float highMidHz = 400.0;
		float lowMidHz = 100.0;
			   
		StereoBiQuad high;
		StereoBiQuad highMid;
		StereoBiQuad lowMid;
		StereoBiQuad low;
		StereoStateVariableFilter highPass;

		json toConfig() {
			json config;

			config["level"] = level;

			config["highGain"] = high.gain;
			config["highMidGain"] = highMid.gain;
			config["lowMidGain"] = lowMid.gain;
			config["lowGain"] = low.gain;
			config["highMidHz"] = highMidHz;
			config["lowMidHz"] = lowMidHz;

			return config;
		}



		void loadConfig(const json cfg) {

			if (cfg.count("level"))
				level = cfg["level"].get<float>();

			if (cfg.count("highGain"))
				high.gain = cfg["highGain"].get<float>();

			if (cfg.count("highMidGain"))
				highMid.gain = cfg["highMidGain"].get<float>();

			if (cfg.count("lowMidGain"))
				lowMid.gain = cfg["lowMidGain"].get<float>();

			if (cfg.count("lowGain"))
				low.gain = cfg["lowGain"].get<float>();

			if (cfg.count("highMidHz"))
				highMidHz = cfg["highMidHz"].get<float>();

			if (cfg.count("lowMidHz"))
				lowMidHz = cfg["lowMidHz"].get<float>();


			highMid.setPeakingEQ(highMidHz);
			lowMid.setPeakingEQ(lowMidHz);

				   
		}



	};

	vector<string> channelNames{ "1","2","3","4","5","6","7","8" };
	
	vector<ChannelEQ> channels;
	vector<future<pair<float, float>>> futures;
	//vector<thread> workers;
	float masterLevel = 1.0f;


	SubMixer(const string name_, const float sampleRate_, shared_ptr<ModMatrix> matrix_, const size_t maxChannels) {
		name = name_;
		sampleRate = sampleRate_;
		matrix = matrix_;

		masterLevel = 1.0f;
		futures.resize(maxChannels);
		//workers.resize(maxChannels);
		channels.resize(maxChannels);
		
		for (int i = 0; i < channels.size(); i++) {
			channels[i].level = 0.0f;
			channels[i].highMidHz = 400.0;
			channels[i].lowMidHz = 100.0;

			channels[i].high.init(sampleRate);
			channels[i].high.setHighShelf(12000.0);

			channels[i].highMid.init(sampleRate);
			channels[i].highMid.setPeakingEQ(channels[i].highMidHz);
			channels[i].lowMid.init(sampleRate);
			channels[i].lowMid.setPeakingEQ(channels[i].lowMidHz);

			channels[i].low.init(sampleRate);
			channels[i].low.setLowShelf(80.0);

			channels[i].highPass.zero(sampleRate);
			channels[i].highPass.setFreq(30.0f);


			//So interface has 1+7*maxChannels elements which we will need to unwind for display
			std::ostringstream name;
			name.str(string());
			name << i << "HighG";
			interfaceList.push_back(EndPoint(true, name.str(), -30.0f, 30.0f, &channels[i].high.gain));
			name.str(string());
			name << i << "HMidR";
			interfaceList.push_back(EndPoint(true, name.str(), 400.0, 8000.0, &channels[i].highMidHz));
			name.str(string());
			name << i << "HMidG";
			interfaceList.push_back(EndPoint(true, name.str(), -30.0f, 30.0f, &channels[i].highMid.gain));
			name.str(string());
			name << i << "LMidR";
			interfaceList.push_back(EndPoint(true, name.str(), 100.0f, 2000.0f, &channels[i].lowMidHz));
			name.str(string());
			name << i << "LMidG";
			interfaceList.push_back(EndPoint(true, name.str(), -30.0f, 30.0f, &channels[i].lowMid.gain));
			name.str(string());
			name << i << "LowG";
			interfaceList.push_back(EndPoint(true, name.str(), -30.0f, 30.0f, &channels[i].low.gain));
			name.str(string());
			name << i << "Lvl";
			interfaceList.push_back(EndPoint(true, name.str(), 0.0f, 1.0f, &channels[i].level));
	
		}

		interfaceList.push_back(EndPoint(true, "Master", 0.0f, 1.0f, &masterLevel));

	}


//Good

//Hmmmm #pragma loop( hint_parallel(4) )

	pair<float, float> mixAudio(list< shared_ptr<AudioUnit> >& channels_) {
		pair<float, float> final = make_pair(0.0f, 0.0f);

		int i = 0;

		auto clambda = [](auto& c, auto& chan, auto& sample) {
			chan.high.setHighShelf(12000.0);  //Reminder this needs to be done for the gain
			chan.highMid.setPeakingEQ(chan.highMidHz);
			chan.lowMid.setPeakingEQ(chan.lowMidHz);
			chan.low.setLowShelf(80.0);      //ditto
			pair<float, float> tmp = make_pair(0.0f, 0.0f);
			c->processAudio(tmp);

			const pair<float, float> h = chan.high.process(tmp);
			const pair<float, float> hm = chan.highMid.process(h);
			const pair<float, float> lm = chan.lowMid.process(h);
			const pair<float, float> l = chan.low.process(make_pair((hm.first + lm.first) * 0.5, (hm.second + lm.second) * 0.5));

			chan.highPass.process(l);
			tmp = chan.highPass.highpass();

			sample.first += get<0>(tmp) * chan.level;
			sample.second += get<1>(tmp) * chan.level;
		};

		for (auto& c : channels_) {
			clambda(c, channels[i], final);
			i++;
		}

		return final;

	}





//Ass
//	pair<float, float> mixAudio(list< unique_ptr<AudioUnit> >& channels_) {
//		pair<float, float> final = make_pair(0.0f, 0.0f);
//			   
//		int i = 0;
//
//		for (auto it = channels_.begin(); it != channels_.end(); it++) {
//			futures[i] = async(launch::async, [](auto& c, auto& chan) {
//				chan.high.setHighShelf(12000.0);  //Reminder this needs to be done for the gain
//				chan.highMid.setPeakingEQ(chan.highMidHz);
//				chan.lowMid.setPeakingEQ(chan.lowMidHz);
//				chan.low.setLowShelf(80.0);      //ditto
//				pair<float, float> tmp(0.0f, 0.0f);
//				
//				c->get()->processAudio(tmp);
//				
//				const pair<float, float> h = chan.high.process(tmp);
//				const pair<float, float> hm = chan.highMid.process(h);
//				const pair<float, float> lm = chan.lowMid.process(h);
//				const pair<float, float> l = chan.low.process(make_pair((hm.first + lm.first) * 0.5, (hm.second + lm.second) * 0.5));
//				
//				chan.highPass.process(l);
//				tmp = chan.highPass.highpass();
//				
//				tmp.first += get<0>(tmp) * chan.level;
//				tmp.second += get<1>(tmp) * chan.level;
//				
//				return tmp;
//				; }, it, channels[i]);
//			i++;
//		}
//
//		for (int f = 0; f < i; f++) {
//			const auto s = futures[f].get();
//			final.first += s.first;
//			final.second += s.second;
//		}
//
//		return final;
//
//	}

//Also ass
//	pair<float, float> mixAudio(list< unique_ptr<AudioUnit> >& channels_) {
//		pair<float, float> final = make_pair(0.0f, 0.0f);
//			   
//		int i = 0;
//
//		for (auto it = channels_.begin(); it != channels_.end(); it++) {
//			workers[i] = thread([](auto& c, auto& chan, auto& sample) {
//				chan.high.setHighShelf(12000.0);  //Reminder this needs to be done for the gain
//				chan.highMid.setPeakingEQ(chan.highMidHz);
//				chan.lowMid.setPeakingEQ(chan.lowMidHz);
//				chan.low.setLowShelf(80.0);      //ditto
//				pair<float, float> tmp(0.0f, 0.0f);
//				
//				c->get()->processAudio(tmp);
//				
//				const pair<float, float> h = chan.high.process(tmp);
//				const pair<float, float> hm = chan.highMid.process(h);
//				const pair<float, float> lm = chan.lowMid.process(h);
//				const pair<float, float> l = chan.low.process(make_pair((hm.first + lm.first) * 0.5, (hm.second + lm.second) * 0.5));
//				
//				chan.highPass.process(l);
//				tmp = chan.highPass.highpass();
//				
//				sample.first += get<0>(tmp) * chan.level;
//				sample.second += get<1>(tmp) * chan.level;
//				
//				; }, it, channels[i], final);
//			i++;
//		}
//
//		for (int w = 0; w < i; w++) {
//			workers[w].join();
//		}
//
//		return final;
//
//	}



	void display() {

		ImGui::PushID(name.c_str());

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1, 4));

		ImGui::PushID("master");
		ImGui::Text("Master");

		for (int i = 0; i < 8; i++) {
			ImGui::SameLine(84 + 210 * i);
			ImGui::Text(channelNames[i].c_str());
		}

		ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor::HSV(0.0f, 0.5f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_SliderGrab, (ImVec4)ImColor::HSV(0.0f, 0.9f, 0.9f));
		ImGui::VSliderFloat("##v", ImVec2(18, 160), &masterLevel, 0.0f, 1.0f, "");
		if (ImGui::IsItemActive() || ImGui::IsItemHovered())
			ImGui::SetTooltip("%.3f", masterLevel);
		ImGui::PopStyleColor(4);
		ImGui::PopID();


		//SameLine(pos_x) hmmmmm

		int interfaceIdx = 0;
		for (int c = 0; c < channels.size(); c++) {
			ImGui::PushID(c);
			for (int i = 0; i < 7; i++) {
				ImGui::PushID(i);
				ImGui::SameLine(44 + 30*interfaceIdx);
				ImGui::PushStyleColor(ImGuiCol_FrameBg,        (ImVec4)ImColor::HSV(0.125f + (0.125f * c), 0.5f, 0.5f));
				ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4)ImColor::HSV(0.125f + (0.125f * c), 0.6f, 0.5f));
				ImGui::PushStyleColor(ImGuiCol_FrameBgActive,  (ImVec4)ImColor::HSV(0.125f + (0.125f * c), 0.7f, 0.5f));
				ImGui::PushStyleColor(ImGuiCol_SliderGrab,     (ImVec4)ImColor::HSV(0.125f + (0.125f * c), 0.9f, 0.9f));

				ImGui::VSliderFloat("##v", ImVec2(18, 160), interfaceList[interfaceIdx].resP, interfaceList[interfaceIdx].min, interfaceList[interfaceIdx].max, "");
				if (ImGui::IsItemActive() || ImGui::IsItemHovered())
					ImGui::SetTooltip(interfaceList[interfaceIdx].name.c_str());
				ImGui::PopStyleColor(4);
				ImGui::PopID();

				interfaceIdx++;
			}
			ImGui::PopID();
		}

		ImGui::NewLine();

		//Next row, m+ buttons

		

		ImGui::SameLine(0);
		mplusButton(interfaceList.size() - 1); //Master level - last element
		
		interfaceIdx = 0;
		for (int c = 0; c < channels.size(); c++) {
			for (int i = 0; i < 7; i++) {
				ImGui::PushID(interfaceIdx);
				ImGui::SameLine(44 + 30 * interfaceIdx);
				if (ImGui::Button("M+")) {
					registerEndPoint(interfaceIdx);
				}
				ImGui::PopID();
				interfaceIdx++;
			}

		}
		
		ImGui::PopStyleVar();

		ImGui::PopID();

		ImGui::End();

	}


	json toConfig() {
		json config = ModuleBase::toConfig();

		config["masterLevel"] = masterLevel;
		
		for (auto& c : channels) {
			config["channels"].push_back(c.toConfig());
		}

		for (auto& n : channelNames) {
			config["channelNames"].push_back(n);
		}

		return config;
	}


	void loadConfig(const json cfg) {

		ModuleBase::loadConfig(cfg);

		if (cfg.count("masterLevel"))
			masterLevel = cfg["masterLevel"].get<float>();

		if (cfg.count("channels")) {
			channels.resize(cfg["channels"].size());
			for (int i = 0; i < cfg["channels"].size(); i++) {
				json ccfg = cfg["channels"][i];
				channels[i].high.init(sampleRate);
				channels[i].highMid.init(sampleRate);
				channels[i].lowMid.init(sampleRate);
				channels[i].low.init(sampleRate);

				channels[i].highPass.zero(sampleRate);
				channels[i].highPass.setFreq(30.0f);

				channels[i].high.setHighShelf(12000.0);
				channels[i].low.setLowShelf(80.0);

				channels[i].loadConfig(ccfg);

			}
		}

		if (cfg.count("channelNames")) {
			channelNames.resize(cfg["channelNames"].size());
			for (int i = 0; i < cfg["channelNames"].size(); i++) {
				channelNames[i] = cfg["channelNames"][i].get<string>();
			}
		}

	}


};

