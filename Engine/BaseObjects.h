#pragma once

#include <memory>
#include <unordered_map>
#include <typeinfo>

#include "Common.h"
#include "Atonal.h"
#include "ModMatrix.h"
#include "SampleManager.h"
#include "Kits.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

//Probably can remove modMatrix from here and change it up to ModBlock and have modMatrix just do stuff on it's own
struct ModuleBase {
protected:
	shared_ptr<ModMatrix> matrix;
	float sampleRate = 44100.0f;
	Interface interface;
	int channel = -1;
//	unordered_map<size_t, uint32_t> registeredEndPoints;

	void registerInterface() {
		matrix->registerInterface(channel, &interface);
	}


public:

	ModuleBase() { ; }
	ModuleBase(const float sampleRate_, shared_ptr<ModMatrix> matrix_, const int channel_) {
		sampleRate = sampleRate_;
		matrix = matrix_;
		channel = channel_;
	}

	~ModuleBase() {
		matrix->removeInterface(channel, &interface);
	}
	
	vector<EndPoint>& getInterface() {
		return interface.consumers;
	}

	vector<EndPoint>& getOptions() {
		return interface.options;
	}

	auto myType() {
		return typeid(*this).name();
	}

	/*
		int id = -1;                     //Change mod matrix and base object to use this rather then returning ids etc
									 //This should make finding end point ids easier
									 //-1 unset
									 //>= 0 Registered in mod matrix
	string name = "";
	float min = 0;
	float max = 0;
	float *resP = nullptr;
	size_t size = 0;
	*/

	virtual json toConfig() { 
		json config;
		config["type"] = myType();

		for (auto& c : interface.consumers) {
			config["interface_consumer_ids"].push_back(c.id);

			json dataj;
			for (int i = 0; i < c.size; i++) {
				dataj.push_back(c.resP[i]);
			}
			config["consumer_data"].push_back(dataj);
		}
		for (auto& p : interface.providers) {
			config["interface_provider_ids"].push_back(p.id);
		}
		for (auto& o : interface.options) {
			config["interface_option_ids"].push_back(o.id);

			json dataj;
			for (int i = 0; i < o.size; i++) {
				dataj.push_back(o.resP[i]);
			}
			config["option_data"].push_back(dataj);
		}



		return config;
	}

	virtual void loadConfig(const json cfg, const bool do_register = false) { 
		//do this by name someday
		if (cfg.count("interface_consumer_ids")) {
			for (int i = 0; i < interface.consumers.size(); i++) {
				interface.consumers[i].id = cfg["interface_consumer_ids"][i].get<int>();
			}
		}
		if (cfg.count("interface_provider_ids")) {
			for (int i = 0; i < interface.providers.size(); i++) {
				interface.providers[i].id = cfg["interface_provider_ids"][i].get<int>();
			}
		}
		if (cfg.count("interface_option_ids")) {
			for (int i = 0; i < interface.options.size(); i++) {
				interface.options[i].id = cfg["interface_option_ids"][i].get<int>();
			}
		}
		
		if (do_register) {
			registerInterface();
		}

		//Now we have destinations to write stuff too
		if (cfg.count("consumer_data")) {
			for (int i = 0; i < interface.consumers.size(); i++) {
				for (int j = 0; j < cfg["consumer_data"][i].size(); j++) {
					interface.consumers[i].resP[j] = cfg["consumer_data"][i][j].get<float>();
				}
			}
		}
		
		if (cfg.count("option_data")) {
			for (int i = 0; i < interface.options.size(); i++) {
				for (int j = 0; j < cfg["option_data"][i].size(); j++) {
					interface.options[i].resP[j] = cfg["option_data"][i][j].get<float>();
				}

			}
		}
	}


};


////For mod matrix sources
struct SignalUnit : ModuleBase {
protected:
	float out = 0.0f;

public:
	SignalUnit() { ; }
	SignalUnit(const float sampleRate_, shared_ptr<ModMatrix> matrix_) {
		sampleRate = sampleRate_;
		matrix = matrix_;
	}

	//virtual void process() = 0;
	virtual void process(const bool tick = false) { ; }

};


//For audio sources
struct AudioUnit : ModuleBase {

public:

	AudioUnit() {;}
	AudioUnit(const float sampleRate_, shared_ptr<ModMatrix> matrix_, const int channel_) {
		sampleRate = sampleRate_;
		matrix = matrix_;
		channel = channel_;
	}
	
	virtual void process_audio(pair<float, float> &in_) { in_ = make_pair(0.0f, 0.0f); }
	
};





//For new audio sources
struct EngineUnit : ModuleBase {

public:

	vector<float> shittyscope;

	EngineUnit() { ; }
	EngineUnit(const float sampleRate_, shared_ptr<ModMatrix> matrix_, const int channel_) {
		sampleRate = sampleRate_;
		matrix = matrix_;
		channel = channel_;
	}

	virtual pair<float, float> process_audio(Kit &kit_, array<float, POLY>& gates_) { return make_pair(0.0f, 0.0f); }

};




//For audio mixers where a different processing interface seems reasonable
//Well that used to be the case - Revisit
struct MixerUnit : ModuleBase {

public:

	MixerUnit() { ; }
	MixerUnit(const float sampleRate_, shared_ptr<ModMatrix> matrix_, const int channel_) {
		sampleRate = sampleRate_;
		matrix = matrix_;
		channel = channel_;
	}

	virtual pair<float, float> mix_audio() { return make_pair(0.0f, 0.0f); }
	//virtual pair<float, float> mix_audio(list< shared_ptr<AudioUnit> > &channels_) { return make_pair(0.0f, 0.0f); }

};
