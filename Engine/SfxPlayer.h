//
//A sample player for playing back game sfx samples
//Maybe should do pcm style playback with transient + repeated section
//certainly some fancier spatial options
//
//intended to be triggered externally
//via functions or queue or whatnot yet to be whatnotted
//
//Things:
//Not limited by POLY playback
//looping samples supported
//should take in a priority
//samples <est in priority gets faded out while others play
//should fade back if room clears up and is still playing
//thinking about position + direction/velocity
//Well position is really just pan, so
//direction -1, 0, 1
//velocity something / s
//once > 1 or < 0 should attenuate volume + keep pan
//
//

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
#include <memory>
#include <list>

#include "BaseObjects.h"
#include "Filter.h"
#include "Delay.h"
#include "Trigger.h"


#include <boost/pool/pool_alloc.hpp>

using namespace std;


//
//Things:
//Not limited by POLY playback
//looping samples supported
//should take in a priority
//samples <est in priority gets faded out while others play
//should fade back if room clears up and is still playing
//thinking about position + direction/velocity
//Well position is really just pan, so
//direction -1, 0, 1
//velocity something / s
//once > 1 or < 0 should attenuate volume + keep pan
//
//
//Some sort of queue
//  pb request
//		add to pb_queue if pb_queue < max_sfx_playback || if priority > pb_queue max priority
//
//   sample a, priority 3, idx 343, false
//   sample b, priority 1, idx 63784, true
//   sample c, priority 4, idx 1837, false
//   sample d, priority 2, idx 138, false
//   -----new sample
//   sample e, priority 5, idx 0, false
//
//Okay so e has passed the 1st test, it's priority is > 4 the old max priority
//So what do? If we sort the list by priority we get
//
//   sample e, priority 5, idx 0, false
//   sample c, priority 4, idx 1837, false
//   sample a, priority 3, idx 343, false
//   sample d, priority 2, idx 138, false
//   sample b, priority 1, idx 63784, true
//
//Any samples in that list positioned > max_sfx_playback
//  move to fade_out_queue
//
//  during pb
//		evaluate pb_queue
//			pb all samples based on position gain etc but modulate gain up to 1 by fade_rate
//			sample idx++
//			if sample idx > sample size
//				done! remove queue item
//				unless looping
//				then set idx = 0
//			set current max priority
//
//		evaluate fade_out_queue
//			pb all samples based on position gain etc but modulate gain down to 0 by fade_rate
//			sample idx++
//			if sample idx > sample size
//				done! remove queue item
//				unless looping
//				then set idx = 0
//			if sample looping && priority > max priority  || pb_queue size < max_sfx_playback
//				move sample to pb_queue
//





struct SfxPlayer : EngineUnit {
private:

	shared_ptr<SampleManager> sample_manager = nullptr;

	float dt = 0.0f;

	int scopeIdx = 0;
	float scopeScale = 1.0f;

	int current_priority_level = 0;
	int max_sfx_playback = 4;

	struct pb_item {
		shared_ptr<Sample> current_sample;
		size_t sample_idx = 0;
		int priority = 0;
		float pan = 0.0f;
		float direction = 0.0f;       //-1.0 0.0 1.0
		float velocity = 0.0f;       // just the x componant
		float level = 1.0f;          // volume adjustment
		float current_level = 0.0f;	 // for fade in / out
		bool loop = false;
		//This seems expensive
		LerpLine<float, 24> delay_l;
		LerpLine<float, 24> delay_r;

	};

	//defaults to allocating 32
	list<pb_item, boost::fast_pool_allocator<pb_item> > pb_queue;
	list<pb_item, boost::fast_pool_allocator<pb_item> > fade_out_queue;

public:

	float fade_rate = 1.0f;

	SfxPlayer(const float sampleRate_, shared_ptr<ModMatrix> matrix_, shared_ptr<SampleManager> sample_manager_, const int channel_) {
		sampleRate = sampleRate_;
		matrix = matrix_;
		sample_manager = sample_manager_;
		channel = channel_;
		dt = 1.0f / sampleRate_;

		shittyscope.resize(1024, 0.0f);
	}

	const int max_playback() {
		return max_sfx_playback;
	}

	void max_playback(const int limit_) {
		max_sfx_playback = limit_;
	}

	//Probably a bad idea to play loops with velocities since they will just play forever
	bool play_sfx(const int priority, const string& sfx_name_, const float pos_ = 0.5f, 
		         const float direction_ = 0.0f, const float velocity_ = 0.0f, const float level_ = 1.0f, const bool loop_ = false) {

		assert(loop_ ? false : velocity_ == 0.0f);
		

		if (pb_queue.size() < max_sfx_playback) {
			pb_queue.push_back(
				{
					sample_manager->getSample(sfx_name_),
					0,
					priority,
					pos_,
					direction_,
					velocity_,
					level_,
					level_,
					loop_
				}
			);

			if(priority > current_priority_level)
				current_priority_level = priority;

			return true;
		}
		

		if (priority > current_priority_level) {
			pb_queue.push_back(
				{
					sample_manager->getSample(sfx_name_),
					0,
					priority,
					pos_,
					direction_,
					velocity_,
					level_,
					level_,
					loop_
				}
			);
			current_priority_level = priority;

			pb_queue.sort([](const pb_item& a, const pb_item& b) { return a.priority > b.priority; });
			auto it = pb_queue.begin();
			advance(it, max_sfx_playback);
			fade_out_queue.splice(fade_out_queue.end(), pb_queue, it, pb_queue.end());

			current_priority_level = 0;
			for_each(pb_queue.begin(), pb_queue.end(), [&](const pb_item& item) {
				if (item.priority > current_priority_level) { current_priority_level = item.priority; }
			});

			return true;
		}
		
		return false;
	}

	
	pair<float, float> process_audio(Kit &kit_, array<float, POLY> & gates_) {
		pair<float, float> sample = make_pair(0.0f, 0.0f);

		auto fade_in = [&](pb_item& item) {
			item.current_level += fade_rate;
			if (item.current_level > item.level)
				item.current_level = item.level;
		};

		auto fade_out = [&](pb_item& item) {
			item.current_level -= fade_rate;
			if (item.current_level < 0.0f)
				item.current_level = 0.0f;
		};

		auto play_queue = [&](pb_item& item) {
			auto current_sample = item.current_sample->get(item.sample_idx++);

			item.delay_l.set(get<0>(current_sample));
			item.delay_r.set(get<1>(current_sample));

			//int priority = 0;
			//float pan = 0.0f;
			//float diection = 0.0f;       //-1.0 0.0 1.0
			//float velocity = 0.0f;       // just the x componant
			//float level = 1.0f;          // volume adjustment
			//float current_level = 0.0f;	 // for fade in / out
			//bool loop = false;
			item.pan += signum<float>(item.direction) * item.velocity * (1.0f / 343.0f) * dt;

			if (item.pan > 1.0f) {
				const auto pan_partial = item.pan - static_cast<int>(item.pan);
				item.pan = 1.0f;
				item.level -= pan_partial;
				if (item.level < 0.0f)
					item.level = 0.0f;
				
			}

			if (item.pan < 0.0f) {
				const auto pan_partial = abs(item.pan - static_cast<int>(item.pan));
				item.pan = 0.0f;
				item.level -= pan_partial;
				if (item.level < 0.0f)
					item.level = 0.0f;
			}
			
			const auto pan_l = item.current_level * panL(item.pan);
			const auto pan_r = item.current_level * panR(item.pan);

			get<0>(sample) += item.delay_l.tap(pan_l) * pan_l;
			get<1>(sample) += item.delay_r.tap(pan_r) * pan_r;
		};

		for_each(pb_queue.begin(), pb_queue.end(), fade_in);
		for_each(fade_out_queue.begin(), fade_out_queue.end(), fade_out);

		for_each(pb_queue.begin(), pb_queue.end(), play_queue);
		for_each(fade_out_queue.begin(), fade_out_queue.end(), play_queue);

		auto finished = [](pb_item& item) {
			if (item.sample_idx > item.current_sample->size()) {
				if (item.loop) {
					item.sample_idx = 0;
					return false;
				}
				return true;
			}
			return false;
		};

		pb_queue.remove_if(finished);
		fade_out_queue.remove_if(finished);

		current_priority_level = 0;
		for_each(pb_queue.begin(), pb_queue.end(), [&](const pb_item& item) {
			if (item.priority > current_priority_level) { current_priority_level = item.priority; }
		});


		//Sadly no splice if
		auto it_fob = fade_out_queue.begin();
		while (it_fob != fade_out_queue.end()) {
			auto prev = it_fob++;
			if (prev->priority > current_priority_level || pb_queue.size() < max_sfx_playback) {
				pb_queue.splice(pb_queue.end(), fade_out_queue, prev);
			}
		}


		shittyscope[scopeIdx] = get<0>(sample);
		scopeIdx++;
		if (scopeIdx >= shittyscope.size()) {
			scopeIdx = 0;
		}

		return sample;

	}

	void loadConfig(const json cfg) {
		ModuleBase::loadConfig(cfg, false);
	}

};



