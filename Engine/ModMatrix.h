#pragma once

#include <vector>
#include <list>
#include <string>
#include <memory>
#include <unordered_map>
#include <map>
#include <typeinfo>
#include <atomic>
#include <chrono>
#include <thread>
#include <limits>
#include <algorithm>
#include <cstdint>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "Common.h"
#include "Atonal.h"
#include "Trigger.h"



using namespace std;


struct EndPoint {
	int id = -1;                     //Change mod matrix and base object to use this rather then returning ids etc
	                                 //This should make finding end point ids easier
	                                 //-1 unset
	                                 //>= 0 Registered in mod matrix
	string name = "";
	float min = 0;
	float max = 0;
	float *resP = nullptr;			  //Maybe should make this + size a span when dynamic_extent comes around in 20
	size_t size = 0;                  //Should be either 1 or poly
	EndPoint() = default;
	EndPoint(const string name, const float min, const float max, float *resP, const size_t size) : name(name), min(min), max(max), resP(resP), size(size) { ; }
};

struct Interface {
	vector<EndPoint> consumers;
	vector<EndPoint> providers;
	vector<EndPoint> options;
};

struct ModMatrix {
private:
	//const vector<float> hues{ 0.3f, 0.7f };
	//array<char, 64> textbuf;
	//bool editName = false;
	//

	struct Connection {
		//		int providerIdx = 0;
		//		int consumerIdx = 0;
		EndPoint* provider = nullptr;
		EndPoint* consumer = nullptr;
		float offset = 0.0f;
		float lastAttenuation = 0.0f;
		float targetAttenuation = 1.0f;
		bool stale = true;
		Connection() = default;
		Connection(EndPoint* provider_, EndPoint* consumer_, const float attenuation_) { 
			provider = provider_;
			consumer = consumer_;
			lastAttenuation = 0.0f; 
			targetAttenuation = attenuation_;
			stale = false;
		}
	};

	struct mMatrix {
		string name;
		//unordered_map<int, Connection> connections;
		vector<Connection> connections;
	};

	struct FreeSlot {
		float* resP = nullptr;
		size_t size = -1;
	};

	//Rename if I don't add more stuff soon
	//struct ChannelElements {
	//	Interface* interface = nullptr;
	//};


	array<vector<Interface*>, CHANNELS+1> channelElements; // +1 stealth channel for the matrix itself
	vector<int> consumerUpdateList;
	list<FreeSlot> free_slots;

	Interface local_interface;                       //We can make ourselves channel C+1, so 8 for now
	int interfaceIdx = -1;


	int next_ep_id = 0;
	size_t next_ep_idx = 0;
	array<float, (CHANNELS * 24 * POLY) + ((CHANNELS+1) * 24) + (CHANNELS * 16 * POLY ) > end_point_data;	//max poly options + max options + modulation stuff
																				//end_point_data[0] -> end_point_data[CHANNELS * 24] option end points
																				//end_point_data[CHANNELS * 24] -> end_point_data[N] Poly sized end points
																				//Not sure if it's better to do that or just allocate stuff as it comes
	//Buffers for copying connection data about
	array<array<float, (POLY + 2)>, 64> buffer;						//poly + min max, max
	array<float, (POLY * 24) + (POLY * 6)> option_buffer;			// poly * max options per channel... 24? seems enough and 6 for the channel eqs

	


	int currentFrame = 0;

	Trigger fwdTrigger;
	Trigger bckTrigger;

	float dt = 0.0f;

	float frameSlewDp = 0.0f;
	float frameSlewPhase = 0.0f;

	atomic<bool> processingConnections = false;

public:

	//Cantor pair
	//uint64_t key(const uint32_t p_, const uint32_t  c_) {
	//	return (((static_cast<uint64_t>(p_) + static_cast<uint64_t>(c_)) * (static_cast<uint64_t>(p_) + static_cast<uint64_t>(c_) + 1)) / 2) + static_cast<uint64_t>(c_);
	//}

	uint64_t key(const uintptr_t p_, const uintptr_t  c_) {
		return (((static_cast<uint64_t>(p_) + static_cast<uint64_t>(c_)) * (static_cast<uint64_t>(p_) + static_cast<uint64_t>(c_) + 1)) / 2) + static_cast<uint64_t>(c_);
	}
	
	float frameSlewTime = 0.0f;
	size_t length = 64;

	array<mMatrix, 64> connectionSet;
	

	vector<EndPoint*> consumers;
	unordered_map<uint32_t, EndPoint*> consumerIdLookup;
	vector<EndPoint*> providers;
	unordered_map<uint32_t, EndPoint*> providerIdLookup;
	vector<EndPoint*> options;
	unordered_map<uint32_t, EndPoint*> optionIdLookup;



	ModMatrix(const float sampleRate_) {

		fill(consumerUpdateList.begin(), consumerUpdateList.end(), 0);

		consumers.reserve(256);
		providers.reserve(256);
		options.reserve(256);
		consumerIdLookup.reserve(256);
		providerIdLookup.reserve(256);
		optionIdLookup.reserve(256);

		consumerUpdateList.reserve(64);

		for (auto& cs : connectionSet) {
			cs.connections.reserve(64);
		}

		next_ep_id = 0;
		next_ep_idx = 0;
		end_point_data.fill(0.0f);
		for (auto& b : buffer) {
			b.fill(0.0f);
		}
		option_buffer.fill(0.0f);

		dt = 1.0f / sampleRate_;

		for (auto& v : channelElements) {
			v.reserve(POLY+1);
		}

		local_interface.providers = {
			{ "Const", 0.0f, 1.0f, nullptr, 1 }
		};
		
		//local_interface.consumers = {
		//	{ "Matrix Triggers", 0.0f, 1.0f, nullptr, 2 } //this will need testing
		//};
		
		registerInterface(POLY+1, &local_interface);

		*local_interface.providers[0].resP = 1.0f;

		processingConnections = false;

	}

private:

	void updateLists() {

		consumers.clear();
		providers.clear();
		options.clear();
		

		for (auto& chan : channelElements) {
			if (chan.size() > 0) {
				for (auto& c : chan) {
					for (auto& con : c->consumers) {
						consumers.push_back(&con);
					}
					for (auto& pro : c->providers) {
						providers.push_back(&pro);
					}
					for (auto& opt : c->options) {
						options.push_back(&opt);
					}
				}
			}

		}

		//sort(consumers.begin(), consumers.end());
		//sort(providers.begin(), providers.end());

	}

	void updateIdLookups() {

		consumerIdLookup.clear();
		providerIdLookup.clear();
		optionIdLookup.clear();

		for (auto& chan : channelElements) {
			if (chan.size() > 0) {
				for (auto& c : chan) {
					for (auto& con : c->consumers) {
						consumerIdLookup[con.id] = &con;
					}
					for (auto& pro : c->providers) {
						providerIdLookup[pro.id] = &pro;
					}
					for (auto& opt : c->options) {
						optionIdLookup[opt.id] = &opt;
					}
				}
			}

		}

	}

public:

	const bool busy() {
		return processingConnections;
	}

	//consumers, providers, options etc
	void allocate_end_points(vector<EndPoint>& interface_element_, unordered_map<uint32_t, EndPoint*>& id_table_) {
		for (int i = 0; i < interface_element_.size(); i++) {
			if (interface_element_[i].id < 0) {
				interface_element_[i].id = next_ep_id;
				next_ep_id++;
			}
			else {
				id_table_[interface_element_[i].id] = &interface_element_[i];
				if (next_ep_id >= interface_element_[i].id)
					next_ep_id = interface_element_[i].id + 1;
			}

			if (free_slots.size() > 0) {
				auto slot_it = std::find_if(free_slots.begin(), free_slots.end(),
					[&size = interface_element_[i].size] 
					(const FreeSlot& fs) -> bool { return fs.size == size; }
				);

				if (slot_it != free_slots.end()) {
					interface_element_[i].resP = slot_it->resP;
					id_table_[interface_element_[i].id] = &interface_element_[i];
					free_slots.erase(slot_it);
					continue;
				}

			}

			//else we  drop through to the default, make a new idx
			interface_element_[i].resP = &end_point_data[next_ep_idx];
			next_ep_idx += interface_element_[i].size;

			id_table_[next_ep_id] = &interface_element_[i];
			next_ep_id++;

		}

	}
	

	


	void registerInterface(const int channel, Interface* interface) {
		assert(interface);
		assert(next_ep_idx < end_point_data.size()); // well at this point we need to de-fragment and shuffle things about
													 // or manage a list of IDs we can re-use
													 // just crash for now and note it here
		//really? hmmm
		if (channel >= 0) {

			channelElements[channel].push_back(interface);
			allocate_end_points(interface->options, optionIdLookup);
			allocate_end_points(interface->consumers, consumerIdLookup);
			allocate_end_points(interface->providers, providerIdLookup);
	
			updateLists();
		}
		else {
			cout << "Channel -1 Registration attempt" << endl;
		}

	}

	//Remove an interface's connections from all connection sets
	//Seems to be failing

	void removeInterface(const int channel, Interface* interface) {

		auto chan_it = std::find_if(channelElements[channel].begin(), channelElements[channel].end(),
			[&iterf = interface]
			(const Interface* i) -> bool { return i == iterf; }
		);


		if (chan_it != channelElements[channel].end()) {
			
			for (auto& o : (*chan_it)->options) {
				free_slots.push_back({ o.resP, o.size });
			}
			for (auto& c : (*chan_it)->consumers) {
				free_slots.push_back({ c.resP, c.size });
			}
			for (auto& p : (*chan_it)->providers) {
				free_slots.push_back({ p.resP, p.size });
			}

			for (auto& frame : connectionSet) {
				//Once this is tested and working flip the search order around so it's connections being searched

				//reminder options are a special type of consumer
				for (auto& o : (*chan_it)->options) {

					auto con_it = std::find_if(frame.connections.begin(), frame.connections.end(),
						[&](const Connection& connection) -> bool { return connection.consumer == &o; }
					);

					if (con_it != frame.connections.end()) {
						con_it->stale = true;
					}

				}

				for (auto& p : (*chan_it)->providers) {

					auto con_it = std::find_if(frame.connections.begin(), frame.connections.end(),
						[&](const Connection& connection) -> bool { return connection.provider == &p; }
					);

					if (con_it != frame.connections.end()) {
						con_it->stale = true;
					}

				}

				for (auto& c : (*chan_it)->consumers) {

					auto con_it = std::find_if(frame.connections.begin(), frame.connections.end(),
						[&](const Connection& connection) -> bool { return connection.consumer == &c; }
					);

					if (con_it != frame.connections.end()) {
						con_it->stale = true;
					}

				}

			}

			channelElements[channel].erase(chan_it);

		}

		updateLists();
		updateIdLookups();

	}

	void setConnection(const int matrixId_, EndPoint* provider_, EndPoint* consumer_, const float attenuation_ = 1.0f) {

		for (auto& c : connectionSet[matrixId_].connections) {
			if (c.provider == provider_) {
				if (c.consumer == consumer_) {

					// found connection, toggle it off

					while (processingConnections == true) {
						std::this_thread::sleep_for(std::chrono::microseconds(100));
					}
				
					c.stale = true;
					c.consumer = nullptr;
					c.provider = nullptr;
					c.lastAttenuation = 0.0f;
					c.targetAttenuation = attenuation_;
					//cout << "Unset connection p: " << reinterpret_cast<uintptr_t>(provider_) << " c: " << reinterpret_cast<uintptr_t>(consumer_) << endl;
					return;
				}
			}
		}

		//Not found
		if (connectionSet[matrixId_].connections.size() < 64) {
			connectionSet[matrixId_].connections.push_back(Connection(provider_, consumer_, attenuation_));
		}
		else {
			for (auto& c : connectionSet[matrixId_].connections) {
				if (c.provider == nullptr || c.consumer == nullptr) {
					c.stale = false;
					c.consumer = consumer_;
					c.provider = provider_;
					c.targetAttenuation = attenuation_;
					//cout << "Set connection p: " << reinterpret_cast<uintptr_t>(provider_) << " c: " << reinterpret_cast<uintptr_t>(consumer_) <<  " a: " << attenuation_ << endl;
				}
			}
		}

	}

	EndPoint* getProviderById(const int id_) {
		if (providerIdLookup.find(id_) != providerIdLookup.end()) {
			return providerIdLookup[id_];
		}
		return nullptr;
	}

	EndPoint* getConsumerById(const int id_) {
		if (consumerIdLookup.find(id_) != consumerIdLookup.end()) {
			return consumerIdLookup[id_];
		}
		return nullptr;
	}

	//This needs a re-think, and should not be operated while audio is running
	void clear() {

		for (int i = 0; i < channelElements.size(); i++) {
			channelElements[i].clear();
		}

		for (auto& c : connectionSet) {
			c.name = "";
			c.connections.clear();
		}

		end_point_data.fill(0.0f);
		next_ep_id = 0;
		next_ep_idx = 0;

		for (auto& a : buffer) {
			for (auto& v : a) {
				v = 0.0f;
			}
		}

		free_slots.clear();

		providers.clear();
		providerIdLookup.clear();
		consumers.clear();
		consumerIdLookup.clear();

		registerInterface(POLY+1, &local_interface); //modMatrix is always 1st yet last

	}

	auto getCurrentFrame() {
		return currentFrame;
	}

	void pasteCurrentFrame(const int paste_) {

		while (processingConnections == true) {
			std::this_thread::sleep_for(std::chrono::microseconds(100)); //was milliseconds(1)
		}

		connectionSet[currentFrame].connections = connectionSet[paste_].connections;

	}


	//Copy data from providers to consumers, total and scale
	//Well that was worth the kerfuffle, 25% cpu time spent in here to 2.5ish
	//The buffer and consumer list seem like a bit of a faff but there you go
	//And I suspect the linear search could be in a smarter place
	//Just filling up to connections.size() then sorting / checking for duplicates
	//seems like it could be quicker
	void Process() {

		processingConnections = true;
		//int cListIdx = 0;

		for (int c = 0; c < connectionSet[currentFrame].connections.size(); c++) {
			auto connection = connectionSet[currentFrame].connections[c];
			auto provider = connection.provider;
			auto consumer = connection.consumer;

			if (connection.stale) //Skip out if the connection has been unset
				continue;

			buffer[c].fill(0.0f); //hummm buffer should be larger then 6 surely

			const auto atten = lerp<float>(connection.lastAttenuation, connection.targetAttenuation, frameSlewPhase);
			//1->poly
			if (provider->size < consumer->size) {
				//int poly_idx = 0;
				//
				//for (int i = 0; i < consumer->size; i++) {
				//	buffer[c][i] += provider->resP[poly_idx] * atten;  //Getting the odd crash here when changing devices, need to check those functions
				//}
				//
				// Causing crashes, getting an i of 86056 then a write exception... gosh really?
				// May be fixed
				//
				cout << "Processing error Provider size: " << provider->size << " " << consumer->size << "\n";
			} else { //poly->poly and 1->1
				if (connection.stale)
					cout << "stale write!!\n";

				for (int i = 0; i < consumer->size; i++) {
					buffer[c][i] += provider->resP[i] * atten;
				}
			}
			

			buffer[c][POLY] += provider->min;
			buffer[c][POLY+1] += provider->max;

			bool found = false;
			for (int i = 0; i < consumerUpdateList.size(); i++) {
				if (consumerUpdateList[i] == c) {
					found = true;
				}

			}

			if (!found) {
				consumerUpdateList.push_back(c);
			}

		}

		//was sloooowwww, now much better
		for (auto& c : consumerUpdateList) {
			auto connection = connectionSet[currentFrame].connections[c];
			auto consumer = connection.consumer;

			const auto min = consumer->min;
			const auto max = consumer->max;
			const auto resMin = buffer[c][POLY];
			const auto resMax = buffer[c][POLY+1];

			//#pragma omp simd safelen(10)
			for (int i = 0; i < POLY; i++) {
				buffer[c][i] = min + (buffer[c][i] - resMin) / (resMax - resMin) * (max - min);
			}

			
			//
			//This will need a breadth copied sort of thing
			// then size < breadth
			// copy
			// 0->breadth -> single dest
			// or average the breadth
			// or combine it
			// or 
			// currently this will just give you lane 0
			// keep in mind min / max going out of wack here
			// So just channel 0 would be okay
			//
			// also the other case single into many
			//
			// so the trick here is to write the smaller values into the buffer
			//
			for (int i = 0; i < consumer->size; i++) {
				consumer->resP[i] = buffer[c][i];
			}

		}

		if (frameSlewTime > 0.0f) {
			frameSlewDp = (1.0f / frameSlewTime) * dt;
			frameSlewPhase += frameSlewDp;
		}
		else {
			frameSlewPhase = 1.0f;
		}


		if (frameSlewPhase > 1.0f) {
			frameSlewPhase = 1.0f;
		}

		processingConnections = false;

		//will need testing
		//if (fwdTrigger.process(local_interface.consumers[0].resP[0])) {
		//	nextFrame();
		//}
		//
		//if (bckTrigger.process(local_interface.consumers[0].resP[1])) {
		//	prevFrame();
		//}

		consumerUpdateList.clear();


	}


	//Needs a rethink, next fram attenuation was a thing...
	void nextFrame() {

		while (processingConnections == true) {
			std::this_thread::sleep_for(std::chrono::microseconds(100)); //was milliseconds(1)
		}

		auto nextFrame = currentFrame + 1;
		if (nextFrame >= length) {
			nextFrame = 0;
		}

		for (int c = 0; c < connectionSet[currentFrame].connections.size(); c++) {
			auto connection = connectionSet[currentFrame].connections[c];
			auto provider = connection.provider;
			auto consumer = connection.consumer;

			auto next_connection = connectionSet[nextFrame].connections[c];
			auto next_provider = next_connection.provider;
			auto next_consumer = next_connection.provider;

			if (provider == nullptr || consumer == nullptr) //Skip out if the connection has been unset
				continue;

			if (next_provider == nullptr || next_consumer == nullptr) //Skip out if the connection will be unset
				continue;

			//If we are all good to this point set copy our current attenuation to the next's current attenuation
			//next_connection.lastAttenuation = connection.lastAttenuation;

			//Test!! It was!
			next_connection.lastAttenuation = lerp<float>(connection.lastAttenuation, connection.targetAttenuation, frameSlewPhase);

			//Note - removed lerp(c.second.lastAttenuation, c.second.targetAttenuation, frameSlewPhase);
			//Not sure if required here


		}

		currentFrame = nextFrame;
		frameSlewPhase = 0.0f;
	}

	//ditto
	void prevFrame() {

		while (processingConnections == true) {
			std::this_thread::sleep_for(std::chrono::microseconds(100)); //was milliseconds(1)
		}

		auto nextFrame = currentFrame - 1;
		if (nextFrame < 0) {
			nextFrame = length - 1;
		}

		for (int c = 0; c < connectionSet[currentFrame].connections.size(); c++) {
			auto connection = connectionSet[currentFrame].connections[c];
			auto provider = connection.provider;
			auto consumer = connection.consumer;

			auto next_connection = connectionSet[nextFrame].connections[c];
			auto next_provider = next_connection.provider;
			auto next_consumer = next_connection.provider;

			if (provider == nullptr || consumer == nullptr) //Skip out if the connection has been unset
				continue;

			if (next_provider == nullptr || next_consumer == nullptr) //Skip out if the connection will be unset
				continue;

			//If we are all good to this point set copy our current attenuation to the next's current attenuation
			//next_connection.lastAttenuation = connection.lastAttenuation;

			//Test!!
			next_connection.lastAttenuation = lerp<float>(connection.lastAttenuation, connection.targetAttenuation, frameSlewPhase);

			//Note - removed lerp(c.second.lastAttenuation, c.second.targetAttenuation, frameSlewPhase);
			//Not sure if required here


		}

		currentFrame = nextFrame;
		frameSlewPhase = 0.0f;
	}


	json toConfig() {
		json config;

		config["frameSlewTime"] = frameSlewTime;
		config["length"] = length;

		for (int i = 0; i < connectionSet.size(); i++) {
			json cSet;
			cSet["name"] = connectionSet[i].name;
	
			for (auto& con : connectionSet[i].connections) {
				json connectionj;
				if (con.provider && con.consumer) {
					if (con.provider->id && con.consumer->id) {
						connectionj["providerIdx"] = con.provider->id;
						connectionj["consumerIdx"] = con.consumer->id;
						connectionj["attenuation"] = con.targetAttenuation;
					}
				}
				cSet["connections"].push_back(connectionj);
			}
			config["connectionSet"].push_back(cSet);
		}

		return config;
	}


	//Assumes clear() has been run before this
	void loadConfig(const json cfg) {
		//We need to re-index the interface because those elements may now have different IDs
		//after things have loaded up their previous interfaces
		updateLists();
		updateIdLookups();
	
		if (cfg.count("frameSlewTime")) {
			frameSlewTime = cfg["frameSlewTime"].get<float>();
		}
	
		if (cfg.count("length")) {
			length = cfg["length"].get<size_t>();
		}
	
		if (cfg.count("connectionSet")) {
			for (int i = 0; i < connectionSet.size(); i++) {
	
				connectionSet[i].name = cfg["connectionSet"][i]["name"].get<string>();
	
				if (cfg["connectionSet"][i].count("connections")) {
					for (auto& cj : cfg["connectionSet"][i]["connections"]) {
						if (cj.count("providerIdx") && cj.count("consumerIdx") && cj.count("attenuation")) {
							setConnection(i, getProviderById(cj["providerIdx"].get<int>()), getConsumerById(cj["consumerIdx"].get<int>()), cj["attenuation"].get<float>());
						}
	
					}
	
				}
	
			}
	
		}
	
	}


















};

