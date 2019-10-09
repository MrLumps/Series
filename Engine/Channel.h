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

#include "BaseObjects.h"
#include "MeasureRunner.h"
#include "ModMatrix.h"
#include "Modulation.h"
#include "WTVoice.h"
#include "ModalVoice.h"
#include "BlockVoice.h"
#include "FmWTVoice.h"
#include "MultiVoice.h"
//#include "Slicer.h"
#include "Sampler.h"
#include "SfxPlayer.h"
#include "Filter.h"
#include "Fx.h"

using namespace std;

//See https://web.archive.org/web/20120204043306/http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt
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
		const double o = 2.0 * PI * hz_ / sampleRate;
		const double A = sqrt(pow(10.0, (static_cast<double>(gain) / 40.0)));
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
		const double o = 2.0 * PI * hz_ / sampleRate;
		const double A = sqrt(pow(10.0, (static_cast<double>(gain) / 40.0)));
		//sin(w0)/2 * sqrt( (A + 1/A)*(1/S - 1) + 2 )
		const double alpha = sin(o) / 2.0f * sqrt((A + 1.0 / A) * (1.0 / Q - 1.0) + 2.0);

		const double cosO = cos(o);
		const double sqrA = 2.0 * sqrt(A) * alpha;

		b[0] = A * ((A + 1.0) - (A - 1.0) * cosO + sqrA);
		b[1] = 2.0 * A * ((A - 1.0) - (A + 1.0) * cosO);
		b[2] = A * ((A + 1.0) - (A - 1.0) * cosO - sqrA);
		a[0] = (A + 1.0) + (A - 1.0) * cosO + sqrA;
		a[1] = -2.0 * ((A - 1.0) + (A + 1.0) * cosO);
		a[2] = (A + 1.0) + (A - 1.0) * cosO - sqrA;

		a[1] /= a[0];
		a[2] /= a[0];
		b[0] /= a[0];
		b[1] /= a[0];
		b[2] /= a[0];
	}

	//Q = shelf slope
	void setHighShelf(const float hz_) {
		const double o = 2.0 * PI * hz_ / sampleRate;
		const double A = sqrt(pow(10.0, (static_cast<double>(gain) / 40.0)));
		//sin(w0)/2 * sqrt( (A + 1/A)*(1/S - 1) + 2 )
		const double alpha = sin(o) / 2.0f * sqrt((A + 1.0 / A) * (1.0 / Q - 1.0) + 2.0);

		const double cosO = cos(o);
		const double sqrA = 2.0 * sqrt(A) * alpha;

		b[0] = A * ((A + 1.0) - (A - 1.0) * cosO + sqrA);
		b[1] = -2.0 * A * ((A - 1.0) - (A + 1.0) * cosO);
		b[2] = A * ((A + 1.0) - (A - 1.0) * cosO - sqrA);
		a[0] = (A + 1.0) + (A - 1.0) * cosO + sqrA;
		a[1] = 2.0 * ((A - 1.0) + (A + 1.0) * cosO);
		a[2] = (A + 1.0) + (A - 1.0) * cosO - sqrA;

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



struct ChannelEQ {
public:

	float level = 0.0;

	float highMidHz = 400.0;
	float lowMidHz = 100.0;

	StereoBiQuad high;
	StereoBiQuad highMid;
	StereoBiQuad lowMid;
	StereoBiQuad low;
	StereoStateVariableFilter outputHp; //this should not require changing

};

struct GateCounter {
	//array<float, POLY> *gatep = nullptr;
	array<Trigger, POLY> gate_triggers;

	int count(array<float, POLY>& gates) {
		int total = 0;
		for (int i = 0; i < gate_triggers.size(); i++) {
			if (gate_triggers[i].process(gates[i])) {
				total++;
			}
		}
		return total;
	}

};





struct Channel : MixerUnit {
	
	enum EngineTypes {
		SAMPLES,
		WT,
		MODAL,
		SLICER,
		BLOCK,
		FMWT,
		MULTI,
		SFX,
	};

	enum FxTypes {
		FILTER,
		CHORUS,
		PHASER,
		DELAY,
		REVERB,
		GRANULAR
	};

	
	bool running = false;
	float level = 0.0f;
	string name = "";
	ChannelEQ eq;
	shared_ptr<Measures> measures = nullptr;
	shared_ptr<SampleManager> sampleManager = nullptr;
	shared_ptr<KitManager> kitManager = nullptr;
	shared_ptr<EngineUnit> audioEngine = nullptr; //I'd be happier with unique but that makes the gui un-able to dynamic cast to get engine options
	unique_ptr<MeasureRunner> measureRunner = nullptr;
	unique_ptr<Modulation> modulation = nullptr;
	list< shared_ptr<AudioUnit> > fxChain;
	GateCounter g_counter;

	int measureToLoad = -1;
	int currentMeasure = -1;

	Kit local_kit;
	int kitToLoad = -1;
	int currentKit = -1;
	bool unload_engine = false; //temp, remove

	array<float, POLY> blank; 

	Channel(const string name_, const float sampleRate_, shared_ptr<ModMatrix> matrix_, const int channel_, shared_ptr<Measures> measures_, shared_ptr<SampleManager> sampleManager_, shared_ptr<KitManager> kitManager_) {
		sampleRate = sampleRate_;
		name = name_;
		matrix = matrix_;
		channel = channel_;
		measures = measures_;
		sampleManager = sampleManager_;
		kitManager = kitManager_;
		local_kit.clear();

		eq.level = 0.0f;
		eq.highMidHz = 400.0;
		eq.lowMidHz = 100.0;

		eq.high.init(sampleRate);
		eq.high.setHighShelf(12000.0);

		eq.highMid.init(sampleRate);
		eq.highMid.setPeakingEQ(eq.highMidHz);
		eq.lowMid.init(sampleRate);
		eq.lowMid.setPeakingEQ(eq.lowMidHz);

		eq.low.init(sampleRate);
		eq.low.setLowShelf(80.0);

		eq.outputHp.zero(sampleRate);
		eq.outputHp.setFreq(30.0f);
		
		for (auto& v : blank) {
			v = 0.0f;
		}


		measureToLoad = -1;
		currentMeasure = -1;

		kitToLoad = -1;
		currentKit = -1;

		modulation = make_unique<Modulation>(sampleRate, matrix, channel);
		measureRunner = make_unique<MeasureRunner>(measures);

		unload_engine = false;

	}


	//running better == false here
	void clear() {
		eq.level = 0.0f;
		eq.highMidHz = 400.0;
		eq.lowMidHz = 100.0;

		eq.high.init(sampleRate);
		eq.high.setHighShelf(12000.0);

		eq.highMid.init(sampleRate);
		eq.highMid.setPeakingEQ(eq.highMidHz);
		eq.lowMid.init(sampleRate);
		eq.lowMid.setPeakingEQ(eq.lowMidHz);

		eq.low.init(sampleRate);
		eq.low.setLowShelf(80.0);

		eq.outputHp.zero(sampleRate);
		eq.outputHp.setFreq(30.0f);


		audioEngine = nullptr;
		modulation = nullptr; //checking to see if not clearing this is causing the overlap in mod-matrix's end point buffer
							  //I suspect that because it wasn't being cleared on big loads but mod-matrix's indexes were
							  //we were getting the over lap

		local_kit.clear();
		currentKit = -1;
		kitToLoad = -1;
		measureToLoad = -1;
		
		fxChain.clear();

		//modulation->clear();
		//measureRunner->clear();

	}

	void mute() {

	}

	void disable_engine() {
		unload_engine = true;
	}

	const int getChannel() {
		return channel;
	}

	void enable() {
		if (audioEngine != nullptr) {
			running = true;
		}
	}

	void disable() {
		running = false;
	}

	void loadMeasure(const int measureIdx_) {
		measureToLoad = measureIdx_;
	}

	void saveMeasure() {
		measureRunner->saveMeasure();
	}

	void loadKit(const int kitIdx_) {
		kitToLoad = kitIdx_;
	}

	void saveKit() {
		if(currentKit >= 0)
			kitManager->saveKit(currentKit, local_kit.toConfig());
	}

	void saveKit(const int new_kit_id_) {
		if (new_kit_id_ >= 0)
			kitManager->saveKit(new_kit_id_, local_kit.toConfig());
	}



	void setSample(const int poly_, shared_ptr<Sample> sample_) { 
		local_kit.current_samples[poly_] = sample_;
		//samplePos[poly_] = numeric_limits<size_t>::max();
	}

	const vector<float>* getSampleData(const int poly_) {
		if (local_kit.current_samples[poly_])
			return local_kit.current_samples[poly_]->getData();
	}

	void setEngine(const int engine) {
		switch (engine) {
		case (int)EngineTypes::SAMPLES:
			audioEngine = make_shared<Sampler>(sampleRate, matrix, channel);
			break;
		case (int)EngineTypes::WT:
			audioEngine = make_shared<WTVoice>(sampleRate, matrix, channel);
			break;
		case (int)EngineTypes::MODAL:
			//audioEngine = make_unique<GrainDrops>(sampleRate, matrix, channel, sampleManager);
			audioEngine = make_shared<ModalVoice>(sampleRate, matrix, channel);
			break;
		case (int)EngineTypes::SLICER:
			//audioEngine = make_shared<Slicer>(sampleRate, matrix, channel, sampleManager);
			break;
		case (int)EngineTypes::BLOCK:
			audioEngine = make_shared<BlockVoice>(sampleRate, matrix, channel);
			break;
		case (int)EngineTypes::FMWT:
			audioEngine = make_shared<FmWTVoice>(sampleRate, matrix, channel);
			break;
		case (int)EngineTypes::MULTI:
			audioEngine = make_shared<MultiVoice>(sampleRate, matrix, channel);
			break;
		case (int)EngineTypes::SFX:
			audioEngine = make_shared<SfxPlayer>(sampleRate, matrix, sampleManager, channel);
			break;
		default:
			break;
		}

	}

	void addFx(const int fx) {
		switch (fx) {

		case (int)FxTypes::FILTER:
			fxChain.push_back(make_unique<FilterEffect>(sampleRate, matrix, channel));
			break;
		case (int)FxTypes::PHASER:
			fxChain.push_back(make_unique<PhaserEffect>(sampleRate, matrix, channel));
			break;
		case (int)FxTypes::CHORUS:
			fxChain.push_back(make_unique<ChorusEffect>(sampleRate, matrix, channel));
			break;
		case (int)FxTypes::DELAY:
			fxChain.push_back(make_unique<DelayEffect>(sampleRate, matrix, channel));
			break;
		case (int)FxTypes::REVERB:
			fxChain.push_back(make_unique<ReverbEffect>(sampleRate, matrix, channel));
			break;
		case (int)FxTypes::GRANULAR:
			fxChain.push_back(make_unique<GranularEffect>(sampleRate, matrix, channel, &local_kit));
			break;
		default:
			break;
		}

	}


	pair<float, float> mix_audio(const bool tick_) {
		pair<float, float> audioSample = make_pair(0.0f, 0.0f);
		array<float, POLY> *gatep = nullptr;

		//A coarser clock tick might be better
		if (measureToLoad != currentMeasure && tick_) {
			measureRunner->loadMeasure(measureToLoad);
			currentMeasure = measureToLoad;
		}
		
		if (kitToLoad != currentKit) {
			//local_kit.loadConfig(kitManager->loadKit(kitToLoad), sampleManager);
			local_kit = kitManager->getKit(kitToLoad);
			currentKit = kitToLoad;
		}

		if (unload_engine) {
			audioEngine = nullptr;
			unload_engine = false;
		}

		if (running) {

			if (currentMeasure >= 0) {
				measureRunner->process(tick_);
				gatep = &measureRunner->gates;
			}
			else {
				gatep = &blank;
			}

			//Hmmm maybe should have this outside in Series.h having it here will lag the mod matrix a sample
			//Since it's audio that just means character right?
			modulation->process(*gatep);

			//Process kit rules here for now as we need to count gates and ticks and such
			//Gruuuuu more triggers? I can probably wrap this in if(tick_) for some savings
			if (tick_) {
				const auto gate_count = g_counter.count(*gatep);
				const auto new_kit = local_kit.process_rules(gate_count, tick_);
				if (new_kit >= 0) {
					local_kit = kitManager->getKit(new_kit);
					currentKit = new_kit;
				}
			}


			if (audioEngine) {
				eq.high.setHighShelf(12000.0);  //Reminder this needs to be done for the gain
				eq.highMid.setPeakingEQ(eq.highMidHz);
				eq.lowMid.setPeakingEQ(eq.lowMidHz);
				eq.low.setLowShelf(80.0);      //ditto

				audioSample = audioEngine->process_audio(local_kit, *(gatep));

				if (fxChain.size() > 0) {
					for (auto& fx : fxChain) {
						fx->process_audio(audioSample);
					}
				}

				const pair<float, float> h = eq.high.process(audioSample);
				const pair<float, float> hm = eq.highMid.process(h);
				const pair<float, float> lm = eq.lowMid.process(h);
				const pair<float, float> l = eq.low.process(make_pair((hm.first + lm.first) * 0.5f, (hm.second + lm.second) * 0.5f));
				
				eq.outputHp.process(l);
				audioSample = eq.outputHp.highpass();

				get<0>(audioSample) *= eq.level;
				get<1>(audioSample) *= eq.level;
			}
		}


		return audioSample;


	}


	json toConfig() {
		json config = ModuleBase::toConfig();

		config["running"] = running;
		config["level"] = level;

		config["highGain"] = eq.high.gain;
		config["highMidGain"] = eq.highMid.gain;
		config["lowMidGain"] = eq.lowMid.gain;
		config["lowGain"] = eq.low.gain;
		config["highMidHz"] = eq.highMidHz;
		config["lowMidHz"] = eq.lowMidHz;


		if (modulation)
			config["modulation"] = modulation->toConfig();

		if (audioEngine)
			config["audioEngine"] = audioEngine->toConfig();

		config["currentKit"] = currentKit;
		config["measure"] = measureRunner->measure_idx;

		for (auto& fx : fxChain) {
			json fxj = fx->toConfig();
			config["fxChain"].push_back(fxj);
		}


		return config;
	}



	void loadConfig(const json cfg) {

		clear();
			   
		ModuleBase::loadConfig(cfg);

		if (cfg.count("level"))
			level = cfg["level"].get<float>();

		if (cfg.count("highGain"))
			eq.high.gain = cfg["highGain"].get<float>();

		if (cfg.count("highMidGain"))
			eq.highMid.gain = cfg["highMidGain"].get<float>();

		if (cfg.count("lowMidGain"))
			eq.lowMid.gain = cfg["lowMidGain"].get<float>();

		if (cfg.count("lowGain"))
			eq.low.gain = cfg["lowGain"].get<float>();

		if (cfg.count("highMidHz"))
			eq.highMidHz = cfg["highMidHz"].get<float>();

		if (cfg.count("lowMidHz"))
			eq.lowMidHz = cfg["lowMidHz"].get<float>();


		eq.highMid.setPeakingEQ(eq.highMidHz);
		eq.lowMid.setPeakingEQ(eq.lowMidHz);


		modulation = make_unique<Modulation>(sampleRate, matrix, channel);
		if (cfg.count("modulation")) {
			modulation->loadConfig(cfg["modulation"]);
		}

		if (cfg.count("audioEngine")) {
			auto audioEj = cfg["audioEngine"];

			if (audioEj.count("type")) {
				if (audioEj["type"].get<string>().compare("struct Sampler") == 0) {
					audioEngine = make_unique<Sampler>(sampleRate, matrix, channel);
					audioEngine->loadConfig(audioEj);
				}

				if (audioEj["type"].get<string>().compare("struct WTVoice") == 0) {
					audioEngine = make_shared<WTVoice>(sampleRate, matrix, channel);
					audioEngine->loadConfig(audioEj);
				}

				if (audioEj["type"].get<string>().compare("struct ModalVoice") == 0) {
					audioEngine = make_shared<ModalVoice>(sampleRate, matrix, channel);
					audioEngine->loadConfig(audioEj);
				}

				//if (audioEj["type"].get<string>().compare("struct Slicer") == 0) {
				//	audioEngine = make_shared<Slicer>(sampleRate, matrix, channel, sampleManager);
				//	audioEngine->loadConfig(audioEj);
				//}

				if (audioEj["type"].get<string>().compare("struct BlockVoice") == 0) {
					audioEngine = make_shared<BlockVoice>(sampleRate, matrix, channel);
					audioEngine->loadConfig(audioEj);
				}

				if (audioEj["type"].get<string>().compare("struct FmWTVoice") == 0) {
					audioEngine = make_shared<FmWTVoice>(sampleRate, matrix, channel);
					audioEngine->loadConfig(audioEj);
				}

				if (audioEj["type"].get<string>().compare("struct MultiVoice") == 0) {
					audioEngine = make_shared<MultiVoice>(sampleRate, matrix, channel);
					audioEngine->loadConfig(audioEj);
				}

				if (audioEj["type"].get<string>().compare("struct SfxPlayer") == 0) {
					audioEngine = make_shared<SfxPlayer>(sampleRate, matrix, sampleManager, channel);
					audioEngine->loadConfig(audioEj);
				}

			}

		}


		if (cfg.count("currentKit"))
			currentKit = cfg["currentKit"].get<int>();

		if (currentKit >= 0) {
			local_kit = kitManager->getKit(currentKit);
			kitToLoad = currentKit;
		}

		if (cfg.count("measure"))
			measureToLoad = cfg["measure"].get<int>();

		if (cfg.count("fxChain")) {
			for (auto& fxj : cfg["fxChain"]) {


				//config["type"]
				if (fxj.count("type")) {
					string t = fxj["type"].get<string>();
					if (t.compare("struct FilterEffect") == 0) {
						fxChain.push_back(make_unique<FilterEffect>(sampleRate, matrix, channel));
						fxChain.back()->loadConfig(fxj);
					}
					if (t.compare("struct PhaserEffect") == 0) {
						fxChain.push_back(make_unique<PhaserEffect>(sampleRate, matrix, channel));
						fxChain.back()->loadConfig(fxj);
					}
					if (t.compare("struct ChorusEffect") == 0) {
						fxChain.push_back(make_unique<ChorusEffect>(sampleRate, matrix, channel));
						fxChain.back()->loadConfig(fxj);
					}
					if (t.compare("struct DelayEffect") == 0) {
						fxChain.push_back(make_unique<DelayEffect>(sampleRate, matrix, channel));
						fxChain.back()->loadConfig(fxj);
					}
					if (t.compare("struct ReverbEffect") == 0) {
						fxChain.push_back(make_unique<ReverbEffect>(sampleRate, matrix, channel));
						fxChain.back()->loadConfig(fxj);
					}

					if (t.compare("struct GranularEffect") == 0) {
						fxChain.push_back(make_unique<GranularEffect>(sampleRate, matrix, channel, &local_kit));
						fxChain.back()->loadConfig(fxj);
					}

				}




			}

		}

		//Run last
		if (cfg.count("running"))
			running = cfg["running"].get<bool>();

	}



};

