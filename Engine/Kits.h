#pragma once
#include <memory>
#include <vector>
#include <array>

#include "Atonal.h"
#include "SampleManager.h"
#include "Rules.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#define NUM_KITS 64

using namespace std;

//
//Kit defaults
//All C's is not that good
//0-11 Major scales  Octave 5
//12-23 Minor scales Octave 5
//40 left...
//
//

extern const array<ChromaticScale, NUM_KITS> default_kit_data;

struct Kit {
	enum class Events {
		GATES,
		TIMER
	};
	
	enum class Actions {
		LOAD,				//Load kit indexed by action_src
		SWAP_NOTES,			//Swaps local kit element[action_src] with local kit element[action_dst] - notes
		SWAP_SAMPLES,		//Swaps local kit element[action_src] with local kit element[action_dst] - samples
		ROTATE_NOTES_FWD,	//Roates the local kit notes/samples indexed by action_src to action_dst by 1
		ROTATE_NOTES_BCK,	//Roates the local kit notes/samples indexed by action_src to action_dst by 1
		ROTATE_SAMPLES_FWD,	//Roates the local kit notes/samples indexed by action_src to action_dst by 1
		ROTATE_SAMPLES_BCK,	//Roates the local kit notes/samples indexed by action_src to action_dst by 1
		TURING_NOTES_KIT,	//On every action change local kit note[action_dst] action_dst loops from action_range_start-action_range_end
							//random note from kit indexed by action_src
							//Maybe should have a couple of these
							//Do one for chord / scale perhaps
							//There should be a chance element in here
							//Eg every range element has N chance to change to random note from kit N
							//And for samples too I guess
		TRANSPOSE_NOTES,	//Transpose local kit note values from action_range_start to action_range_end by action_src
		TRANSPOSE_OCTAVES,	//Transposes local kit note octaves from action_range_start to action_range_end by action_src
		//WRITE_SCALE,		//Write atonal::Scale of action_src type action_dst rootN in range action_range_start..action_range_end, needs to wrap octaves
		WRITE_CHORD,		//Write atonal::ChromaticSeventh of action_src type action_dst rootN in range action_range_start..action_range_end, needs to wrap octaves
							//RiemannianTriads?
		ENABLE_RULE,		//Enable rule indexed in action_src
		DISABLE_RULE,		//Disable rule indexed in action_src
		TOGGLE_RULE			//Toggle rule indexed in action_src
	};



	BaseOctave tuning;
	array<Note, POLY> notes;                    
	array<shared_ptr<Sample>, POLY> current_samples; 
	vector<Rule<Events, Actions>> rules;


	Kit& operator=(const Kit& other)  {
		if (this != &other) {
			this->tuning.base_frequency = other.tuning.base_frequency;
			this->tuning.type = other.tuning.type;
			this->notes = other.notes;
			this->current_samples = other.current_samples;
			this->rules = other.rules;
		}
		return *this;
	}

	void clear() {
		tuning.type = TuningType::EQUAL_TEMPERED;
		tuning.base_frequency = 8.1757989156f;
		notes.fill({ 60 });
		current_samples.fill(nullptr);
	}

	void clear(int idx_) {
		tuning.type = TuningType::EQUAL_TEMPERED;
		tuning.base_frequency = 8.1757989156f;
		write_scale(default_kit_data[idx_].tonic, default_kit_data[idx_].scale, 0, POLY-1); 
		current_samples.fill(nullptr);
	}


	void write_scale(const Note root_, const ChromaticScale::ScaleTypes type_, const int start_, const int end_) {
		auto scale = ChromaticScale(root_, type_);
		int scale_idx = 0;
		for (int i = start_; i <= end_; i++) {
			notes[i] = scale[scale_idx];
			scale_idx++;
			if (scale_idx == 7) {
				scale_idx = 0;
				scale.tonic.n += 12;  //step up an octave when we start again
			}
		}
	}

	void write_chord(const Note root_, const ChromaticSeventh::Types type_, const int start_, const int end_) {
		auto chord = ChromaticSeventh(root_, type_);
		int chord_idx = 0;
		for (int i = start_; i <= end_; i++) {
			notes[i] = chord[chord_idx];
			chord_idx++;
			if (chord_idx == 4) {
				chord_idx = 0;
				chord.root.n += 12;  //step up an octave when we start again
			}
		}
	}

	//Fuuuu I totally forgot about event_src
	//that can totally index a particular gate
	//that is probably better then counting all gates
	//though it'll mean we'll need a Trigger in here :/
	int process_rules(const int gate_count_, const bool tick_) {
		for (auto& rule : rules) {
			if (rule.run) {

				switch (rule.event) {
				case Kit::Events::GATES:
					rule.event_counter += gate_count_;
					break;
				case Kit::Events::TIMER:
					if (tick_)
						rule.event_counter++;
					break;
				default:
					break;
				}

				//Want this in kit!!
				if (rule.event_counter >= rule.action_threashold) {

					rule.event_counter = 0;

					switch (rule.action) {
					case Kit::Actions::LOAD:                        //This is problematic, what if the new kit has no rules? either way rules will be smashed
																	//and this will cause a cash.
																	//What we need to do is load the new kit and stop processing rules
																	//I think it's fine if the new rules don't get started till next frame
																	//That means that the beat that triggered the rule doesn't belong to the next rule
																	//which seems sane
						//auto kit_idx = rule.action_src;
						//local_kit = kitManager->getKit(kit_idx);   //rule goes poof
						//currentKit = kit_idx;

						return rule.action_src;
					case Kit::Actions::SWAP_NOTES:					//Swaps local kit element[action_src] with local kit element[action_dst] - notes

						iter_swap(next(notes.begin(), rule.action_src), next(notes.begin(), rule.action_dst));
						break;
					case Kit::Actions::SWAP_SAMPLES:
						iter_swap(next(current_samples.begin(), rule.action_src), next(current_samples.begin(), rule.action_dst));
						break;
					case Kit::Actions::ROTATE_NOTES_FWD:
						rotate(next(notes.begin(), rule.action_src), next(notes.begin(), (long long)rule.action_src + 1), next(notes.begin(), (long long)rule.action_dst+1));
						break;
					case Kit::Actions::ROTATE_NOTES_BCK:
						rotate(next(notes.begin(), rule.action_src), next(notes.begin(), (long long)rule.action_dst - 1), next(notes.begin(), (long long)rule.action_dst+1));
						break;
					case Kit::Actions::ROTATE_SAMPLES_FWD:
						rotate(next(current_samples.begin(), rule.action_src), next(current_samples.begin(), (long long)rule.action_src + 1), next(current_samples.begin(), (long long)rule.action_dst+1));
						break;
					case Kit::Actions::ROTATE_SAMPLES_BCK:
						rotate(next(current_samples.begin(), rule.action_src), next(current_samples.begin(), (long long)rule.action_dst - 1), next(current_samples.begin(), (long long)rule.action_dst+1));
						break;
					case Kit::Actions::TURING_NOTES_KIT:
						break;
					case Kit::Actions::TRANSPOSE_NOTES:
						for (auto& n : notes) {
							n.n += rule.action_src;

							//128 is probably too large for comfort but should stop unexpected math explosions
							//so 108 which is c8 for now
							if (n.n > 108)
								n.n -= 108;
							if (n.n < 0)
								n.n += 108;
						}
						break;
					case Kit::Actions::TRANSPOSE_OCTAVES:
						for (auto& n : notes) {
							n.octave(n.octave() + rule.action_src);
						}
						break;
					case Kit::Actions::WRITE_CHORD:
						//Write atonal::Scale of action_src type action_dst rootN in range action_range_start..action_range_end, needs to wrap octaves
						//write_chord(const Note root_, const ChromaticSeventh::Types type_, const int start_, const int end_) {
						write_chord(Note(rule.action_src), static_cast<ChromaticSeventh::Types>(rule.action_dst), rule.action_range_start, rule.action_range_end);
						break;
					case Kit::Actions::ENABLE_RULE:
						if (rule.action_src < rules.size()) {
							rules[rule.action_src].run = true;
						}
						break;
					case Kit::Actions::DISABLE_RULE:
						if (rule.action_src < rules.size()) {
							rules[rule.action_src].run = false;
							rules[rule.action_src].event_counter = 0;
						}
						break;
					case Kit::Actions::TOGGLE_RULE:
						if (rule.action_src < rules.size()) {
							rules[rule.action_src].run = !rules[rule.action_src].run;
							rules[rule.action_src].event_counter = 0;
						}
						break;
					}
				}
			}
		}

		return -1;

	}








	virtual json toConfig() {
		json config;

		config["tuning_base_frequency"] = tuning.base_frequency;
		
		config["tuning_type"] = tuning.type;

		for (auto& n : notes) {
			config["notes"].push_back(n.n);
		}

		for (int i = 0; i < current_samples.size(); i++) {
			if (current_samples[i]) {
				config["samples"].push_back(current_samples[i]->getName());
			}
			else {
				config["samples"].push_back("");
			}

			
		}

		for (auto& r : rules) {
			config["rules"].push_back(r.toConfig());
		}

		return config;
	}

	void loadConfig(const json cfg, shared_ptr<SampleManager> sampleCache_) {
		clear();

		if (cfg.count("tuning_base_frequency"))
			tuning.base_frequency = cfg["tuning_base_frequency"].get<float>();

		if (cfg.count("tuning_type"))
			tuning.type = static_cast<TuningType>(cfg["tuning_type"].get<int>());

		for (int i = 0; i < POLY; i++) { 
			if(cfg.count("notes"))
				notes[i].n = cfg["notes"][i].get<int>();
			if (cfg.count("samples"))
				current_samples[i] = sampleCache_->getSample(cfg["samples"][i].get<string>());
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


struct KitManager {
private:

	shared_ptr<SampleManager> sampleManager;
	array<Kit, NUM_KITS> kits;

public:

	KitManager(shared_ptr<SampleManager> sampleManager_) {
		sampleManager = sampleManager_;

		for (int i = 0; i < NUM_KITS; i++) {
			kits[i].clear(i);
		}
			
	};


	void saveKit(const int kIdx_, json cfg_) {
		assert(kIdx_ < NUM_KITS);
		kits[kIdx_].loadConfig(cfg_, sampleManager);
	}

	json loadKit(const int kIdx_) {
		assert(kIdx_ < NUM_KITS);
		return kits[kIdx_].toConfig();
	}

	Kit& getKit(const int kIdx_) {
		assert(kIdx_ < NUM_KITS);
		return kits[kIdx_];
	}

	virtual json toConfig() {
		json config;
		
		for (int i = 0; i < NUM_KITS; i++) {
			json kj = kits[i].toConfig();
			config["kits"].push_back(kj);
		}

		return config;
	}

	void loadConfig(const json cfg) {
		if (cfg.count("kits")) {
			for (int i = 0; i < NUM_KITS; i++) {
				json kj = cfg["kits"][i];
				kits[i].loadConfig(kj, sampleManager);
			}
		}
	}

	const size_t size() {
		return NUM_KITS;
	}

};









