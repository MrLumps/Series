#pragma once
#include <memory>
#include <list>
#include <vector>
#include <array>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#define NUM_MEASURE 64

using namespace std;


struct Measure {
	enum class Type {
		EMPTY,
		STEP,
		MIDI,
		LIFE
	};

	Type type = Type::EMPTY;
	json config;

};

struct Measures {
private:

	array<Measure, NUM_MEASURE> measures;

public:

	Measures() {
		for (auto& m : measures) {
			m.type = Measure::Type::EMPTY;
		}
	};


	Measure::Type getMeasureType(const int mIdx_) {
		return measures[mIdx_].type;
	}

	void setMeasureType(const int mIdx_, const Measure::Type t_) {
		measures[mIdx_].type = t_;
	}

	void saveMeasure(const int mIdx_, json cfg_) {
		measures[mIdx_].config = cfg_;
	}

	json loadMeasure(const int mIdx_) {
		return measures[mIdx_].config;
	}

	virtual json toConfig() {
		json config;
		
		for (int i = 0; i < NUM_MEASURE; i++) {
			json mj;
			mj["type"] = measures[i].type;
			mj["config"].push_back(measures[i].config);
			config["measure"].push_back(mj);
		}

		return config;
	}

	void loadConfig(const json cfg) {
		for (int i = 0; i < NUM_MEASURE; i++) {
			const auto& mj = cfg["measure"][i];

			measures[i].type = static_cast<Measure::Type>(mj["type"].get<int>());
			measures[i].config = mj["config"][0]; //well that took a bit of finding
		}
	}

	const size_t size() {
		return NUM_MEASURE;
	}

};











