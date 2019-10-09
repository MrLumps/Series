#pragma once



//#include <nlohmann/json.hpp>
//using json = nlohmann::json;
//
//
//enum class KitEvents {
//	GATES,
//	TIMER
//};
//
//enum class KitActions {
//	LOAD_KIT,		//Load kit indexed by action_src
//	SWAP_KIT,		//Swaps local kit element[action_src] with local kit element[action_dst]
//	ROTATE_KIT_FWD,	//Roates the local kit notes/samples indexed by action_src to action_dst by 1
//	ROTATE_KIT_BCK,	//Roates the local kit notes/samples indexed by action_src to action_dst by 1
//	TURING,			//On every action change local kit note[action_dst] action_dst loops from action_range_start-action_range_end
//	//random note from kit indexed by action_src
//	TRANSPOSE_NOTE,	//Transpose local kit note from action_range_start to action_range_end by action_src
//	WRITE_SCALE,	//Write atonal::Scale of action_src type action_dst rootN in range action_range_start..action_range_end, needs to wrap octaves
//	WRITE_CHORD,	//Write atonal::ChromaticSeventh of action_src type action_dst rootN in range action_range_start..action_range_end, needs to wrap octaves
//					//RiemannianTriads?
//	ENABLE_RULE,	//Enable rule indexed in action_src
//	DISABLE_RULE    //Disable rule indexed in action_src
//};


#include <nlohmann/json.hpp>
using json = nlohmann::json;

//
// Container for common rule stuff, probably best not to use floats/doubles as events / actions
// as there's some type conversion being done in the json stuff being made on that assumption
//



template<typename Events, typename Actions>
struct Rule {
	bool run = false;
	Events event = static_cast<Events>(0);
	int event_src = 0;
	int event_counter = 0;
	int action_threashold = 0;
	Actions action = static_cast<Actions>(0);
	int action_src = 0;
	int action_dst = 0;
	int action_range_start = 0;
	int action_range_end = 0;

	void clear() {
		run = false;
		event = static_cast<Events>(0);
		event_src = 0;
		event_counter = 0;
		action_threashold = 0;
		action = static_cast<Actions>(0);
		action_src = 0;
		action_dst = 0;
		action_range_start = 0;
		action_range_end = 0;
	}

	virtual json toConfig() {
		json config;

		config["run"] = run;
		config["event"] = event;
		config["event_src"] = event_src;
		config["action_threashold"] = action_threashold;
		config["action"] = action;
		config["action_src"] = action_src;
		config["action_dst"] = action_dst;
		config["action_range_start"] = action_range_start;
		config["action_range_end"] = action_range_end;

		return config;
	}

	void loadConfig(const json cfg) {
		//clear(); //should not be required, maybe set for safety... hmm

		if (cfg.count("run"))
			run = cfg["run"].get<bool>();

		if (cfg.count("event"))
			event = static_cast<Events>(cfg["event"].get<int>());

		if (cfg.count("event_src"))
			event_src = cfg["event_src"].get<int>();

		if (cfg.count("action_threashold"))
			action_threashold = cfg["action_threashold"].get<int>();

		if (cfg.count("action"))
			action = static_cast<Actions>(cfg["action"].get<int>());

		if (cfg.count("action_src"))
			action_src = cfg["action_src"].get<int>();

		if (cfg.count("action_dst"))
			action_dst = cfg["action_dst"].get<int>();

		if (cfg.count("action_range_start"))
			action_range_start = cfg["action_range_start"].get<int>();

		if (cfg.count("action_range_end"))
			action_range_end = cfg["action_range_end"].get<int>();

	}


};