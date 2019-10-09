//Currently a hardcoded wrapper for Libsndfile
//Needs a bunch of work, should be defined externally by the user and passed in


#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <memory>
#include <assert.h>

#include "Common.h"
#include <sndfile.hh>

using namespace std;

struct Sample {
private:
	size_t length;
	vector<float> bufferL;
	vector<float> bufferR;
	string name;

public:
	
	Sample(const string filename_) {
		SndfileHandle file;
		std::vector<float> tmp;

		name = filename_;
		file = SndfileHandle(filename_.c_str());

		cout << "loadSample - " << filename_ << endl;

		if (!file.error()) {
			cout << " channels " << file.channels() << " samplingRate " << file.samplerate() << " Format " << hex << file.format() << dec << std::endl;
			cout << " samples " << file.frames() << endl;

			bufferL.resize(file.frames(), 0.0f);
			bufferR.resize(file.frames(), 0.0f);

			fill(bufferL.begin(), bufferL.end(), 0.0f);
			fill(bufferR.begin(), bufferR.end(), 0.0f);
			
			length = file.frames();
			tmp.resize(file.frames() * file.channels());

			file.readf(tmp.data(), file.frames());

			//Left channel only
			if (file.channels() == 2) {
				size_t tmpIdx = 0;
				for (int c = 0; c < file.frames(); c++) {
					bufferL[c] = tmp[tmpIdx];
					bufferR[c] = tmp[tmpIdx + 1];
					tmpIdx += 2;
				}
			}
			else {
				for (int c = 0; c < tmp.size(); c++) {
					bufferL[c] = tmp[c];
					bufferR[c] = tmp[c];
				}
			}
		}
		else {
			std::cout << filename_ << " sndfile Error " << file.strError() << std::endl;
			length = 0;
		}
	}


	size_t size() {
		return length;
	}

	inline float getMono(const size_t pos) {
		assert(pos < length);
		return bufferL[pos];
	}

	inline pair<float, float> get(const size_t pos) {
		return make_pair(bufferL[pos], bufferR[pos]);
	}

	 inline pair<float, float> operator[](const size_t pos) {
		 return make_pair(bufferL[pos], bufferR[pos]);
	}

	 vector<float>* getData() {
		 return &bufferL;
	 }

	 vector<float>* getDataL() {
		 return &bufferL;
	 }

	 vector<float>* getDataR() {
		 return &bufferR;
	 }

	 const string getName() {
		 return name;
	 }


};



struct SampleManager {

private:

	//std::vector<std::string> sampleList = {
	//"Samples/Breaks/tighten_up.wav",
	//"Samples/Breaks/Tighten_Up1.wav",
	//"Samples/Breaks/Tighten_Up2.wav",
	//"Samples/ClosedHats/LM-1/HiHat10.wav",
	//"Samples/Kick/BigOldKick.wav",
	//"Samples/Kick/BigOldKickLong.wav",
	//"Samples/Percussion/DMX/23_RIMSHOT.wav",
	//"Samples/Percussion/EstecDrums/estecdrumspk.aif",
	//"Samples/Percussion/EstecDrums/estecdrumspk10.aif",
	//"Samples/Percussion/EstecDrums/estecdrumspk11.aif",
	//"Samples/Percussion/EstecDrums/estecdrumspk12.aif",
	//"Samples/Percussion/EstecDrums/estecdrumspk13.aif",
	//"Samples/Percussion/EstecDrums/estecdrumspk14.aif",
	//"Samples/Percussion/EstecDrums/estecdrumspk15.aif",
	//"Samples/Percussion/EstecDrums/estecdrumspk16.aif",
	//"Samples/Percussion/EstecDrums/estecdrumspk2.aif",
	//"Samples/Percussion/EstecDrums/estecdrumspk3.aif",
	//"Samples/Percussion/EstecDrums/estecdrumspk4.aif",
	//"Samples/Percussion/EstecDrums/estecdrumspk5.aif",
	//"Samples/Percussion/EstecDrums/estecdrumspk6.aif",
	//"Samples/Percussion/EstecDrums/estecdrumspk7.aif",
	//"Samples/Percussion/EstecDrums/estecdrumspk8.aif",
	//"Samples/Percussion/EstecDrums/estecdrumspk9.aif"
	//};

	
	std::vector<std::string> sampleList = {
	"Samples/Breaks/action.wav",
	"Samples/Breaks/amen_brother-Dirty.wav",
	"Samples/Breaks/amen_brother.wav",
	"Samples/Breaks/amen_brotherA.wav",
	"Samples/Breaks/amen_brotherB.wav",
	"Samples/Breaks/apache.wav",
	"Samples/Breaks/assembly_line.wav",
	"Samples/Breaks/both_eyes_open.wav",
	"Samples/Breaks/cold_sweat.wav",
	"Samples/Breaks/dont_change_your_love.wav",
	"Samples/Breaks/do_the_do.wav",
	"Samples/Breaks/flow.wav",
	"Samples/Breaks/fools_gold.wav",
	"Samples/Breaks/funky_drummer.wav",
	"Samples/Breaks/funky_mule--intro.wav",
	"Samples/Breaks/funky_mule--rides.wav",
	"Samples/Breaks/give_it_up_or_turnit_a_loose.wav",
	"Samples/Breaks/god_made_me_funky.wav",
	"Samples/Breaks/hot_pants.wav",
	"Samples/Breaks/humpty_dump.wav",
	"Samples/Breaks/its_a_new_day.wav",
	"Samples/Breaks/kick_back.wav",
	"Samples/Breaks/kool_is_back.wav",
	"Samples/Breaks/let_a_woman_be_a_woman.wav",
	"Samples/Breaks/life_could.wav",
	"Samples/Breaks/new_orleans.wav",
	"Samples/Breaks/nt.wav",
	"Samples/Breaks/ode_to_billy_joe.wav",
	"Samples/Breaks/plastic_jam.wav",
	"Samples/Breaks/put_your_love--end.wav",
	"Samples/Breaks/put_your_love--middle.wav",
	"Samples/Breaks/scorpio.wav",
	"Samples/Breaks/sesame_street.wav",
	"Samples/Breaks/shack_up.wav",
	"Samples/Breaks/sing_a_simple_song.wav",
	"Samples/Breaks/sneakin_in_the_back.wav",
	"Samples/Breaks/sniper.wav",
	"Samples/Breaks/soul_beat_runna.wav",
	"Samples/Breaks/soul_pride.wav",
	"Samples/Breaks/sport.wav",
	"Samples/Breaks/sweet_pea.wav",
	"Samples/Breaks/take_me_to_the_mardi_gras.wav",
	"Samples/Breaks/think--all5.wav",
	"Samples/Breaks/TightenUp1.wav",
	"Samples/Breaks/TightenUp2.wav",
	"Samples/Breaks/tighten_up.wav",
	"Samples/Breaks/whole_thing.wav",
	"Samples/Breaks/willie_pass_the_water.wav",
	"Samples/Breaks/worm.wav",
	"Samples/ClosedHats/DMX/09_HI-HAT_CLOSED.wav",
	"Samples/ClosedHats/DMX/10_HI-HAT_ACCENT.wav",
	"Samples/ClosedHats/LM-1/HiHat1.wav",
	"Samples/ClosedHats/LM-1/HiHat10.wav",
	"Samples/ClosedHats/LM-1/HiHat11.wav",
	"Samples/ClosedHats/LM-1/HiHat12.wav",
	"Samples/ClosedHats/LM-1/HiHat13.wav",
	"Samples/ClosedHats/LM-1/HiHat2.wav",
	"Samples/ClosedHats/LM-1/HiHat3.wav",
	"Samples/ClosedHats/LM-1/HiHat4.wav",
	"Samples/ClosedHats/LM-1/HiHat5.wav",
	"Samples/ClosedHats/LM-1/HiHat6.wav",
	"Samples/ClosedHats/LM-1/HiHat7.wav",
	"Samples/ClosedHats/LM-1/HiHat8.wav",
	"Samples/ClosedHats/LM-1/HiHat9.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_01.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_02.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_03.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_04.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_05.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_06.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_07.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_08.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_09.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_10.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_11.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_12.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_13.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_14.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_15.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_16.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_17.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_18.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_19.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_20.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_21.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_22.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_23.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_24.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_25.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_26.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_27.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_28.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_29.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_30.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_31.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_32.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_33.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_34.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_35.wav",
	"Samples/ClosedHats/MasterRhythm/SM-8_CLOSED_HI-HAT_36.wav",
	"Samples/Cymbal/DMX/18_RIDE_01.wav",
	"Samples/Cymbal/DMX/19_RIDE_02.wav",
	"Samples/Cymbal/DMX/20_CRASH.wav",
	"Samples/Cymbal/MasterRythm/SM-8_CYMBAL_01.wav",
	"Samples/Cymbal/MasterRythm/SM-8_CYMBAL_02.wav",
	"Samples/Cymbal/MasterRythm/SM-8_CYMBAL_03.wav",
	"Samples/Cymbal/MasterRythm/SM-8_CYMBAL_04.wav",
	"Samples/Cymbal/MasterRythm/SM-8_CYMBAL_05.wav",
	"Samples/Cymbal/MasterRythm/SM-8_CYMBAL_06.wav",
	"Samples/Cymbal/MasterRythm/SM-8_CYMBAL_07.wav",
	"Samples/Cymbal/MasterRythm/SM-8_CYMBAL_08.wav",
	"Samples/Kick/BigOldKick.wav",
	"Samples/Kick/BigOldKickLong.wav",
	"Samples/Kick/DMX/03_BASS_01.wav",
	"Samples/Kick/DMX/04_BASS_02.wav",
	"Samples/Kick/DMX/05_BASS_03.wav",
	"Samples/Kick/LM-1/Kick1.wav",
	"Samples/Kick/LM-1/Kick2.wav",
	"Samples/Kick/LM-1/Kick3.wav",
	"Samples/Kick/LM-1/Kick4.wav",
	"Samples/Kick/LM-1/Kick5.wav",
	"Samples/Kick/LM-1/Kick6.wav",
	"Samples/Kick/LM-1/Kick7.wav",
	"Samples/Kick/LM-1/Kick8.wav",
	"Samples/Kick/MasterRhythm/SM-8_KICK_01.wav",
	"Samples/Kick/MasterRhythm/SM-8_KICK_02.wav",
	"Samples/Kick/MasterRhythm/SM-8_KICK_03.wav",
	"Samples/Kick/MasterRhythm/SM-8_KICK_04.wav",
	"Samples/Kick/MasterRhythm/SM-8_KICK_05.wav",
	"Samples/Kick/MasterRhythm/SM-8_KICK_06.wav",
	"Samples/Kick/MasterRhythm/SM-8_KICK_07.wav",
	"Samples/Kick/MasterRhythm/SM-8_KICK_08.wav",
	"Samples/Kick/MasterRhythm/SM-8_KICK_09.wav",
	"Samples/Kick/MasterRhythm/SM-8_KICK_10.wav",
	"Samples/Kick/MasterRhythm/SM-8_KICK_11.wav",
	"Samples/Kick/MasterRhythm/SM-8_KICK_12.wav",
	"Samples/Kick/MasterRhythm/SM-8_KICK_13.wav",
	"Samples/Kick/MasterRhythm/SM-8_KICK_14.wav",
	"Samples/Kick/MasterRhythm/SM-8_KICK_15.wav",
	"Samples/Kick/MasterRhythm/SM-8_KICK_16.wav",
	"Samples/OpenHats/DMX/11_HI-HAT_OPEN.wav",
	"Samples/OpenHats/LM-1/HiHatOpen1.wav",
	"Samples/OpenHats/LM-1/HiHatOpen2.wav",
	"Samples/OpenHats/LM-1/HiHatOpen3.wav",
	"Samples/OpenHats/LM-1/HiHatOpen4.wav",
	"Samples/OpenHats/LM-1/HiHatOpen5.wav",
	"Samples/OpenHats/LM-1/HiHatOpen6.wav",
	"Samples/OpenHats/LM-1/HiHatOpen7.wav",
	"Samples/OpenHats/LM-1/HiHatOpen8.wav",
	"Samples/Percussion/DMX/01_METRONOME_01.wav",
	"Samples/Percussion/DMX/02_METRONOME_02.wav",
	"Samples/Percussion/DMX/21_TAMB_01.wav",
	"Samples/Percussion/DMX/22_TAMB_02.wav",
	"Samples/Percussion/DMX/23_RIMSHOT.wav",
	"Samples/Percussion/DMX/24_SHAKER_01.wav",
	"Samples/Percussion/DMX/25_SHAKER_02.wav",
	"Samples/Percussion/DMX/26_CLAP.wav",
	"Samples/Percussion/DMX/27_BEEP.wav",
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
	"Samples/Percussion/EstecDrums/estecdrumspk9.aif",
	"Samples/Percussion/LM-1/Cabasa1.wav",
	"Samples/Percussion/LM-1/Cabasa2.wav",
	"Samples/Percussion/LM-1/Cabasa3.wav",
	"Samples/Percussion/LM-1/Cabasa4.wav",
	"Samples/Percussion/LM-1/Cabasa5.wav",
	"Samples/Percussion/LM-1/Cabasa6.wav",
	"Samples/Percussion/LM-1/Cabasa7.wav",
	"Samples/Percussion/LM-1/Cabasa8.wav",
	"Samples/Percussion/LM-1/Cabasa9.wav",
	"Samples/Percussion/LM-1/Clap1.wav",
	"Samples/Percussion/LM-1/Clap10.wav",
	"Samples/Percussion/LM-1/Clap2.wav",
	"Samples/Percussion/LM-1/Clap3.wav",
	"Samples/Percussion/LM-1/Clap4.wav",
	"Samples/Percussion/LM-1/Clap5.wav",
	"Samples/Percussion/LM-1/Clap6.wav",
	"Samples/Percussion/LM-1/Clap7.wav",
	"Samples/Percussion/LM-1/Clap8.wav",
	"Samples/Percussion/LM-1/Clap9.wav",
	"Samples/Percussion/LM-1/CowBell1.wav",
	"Samples/Percussion/LM-1/CowBell10.wav",
	"Samples/Percussion/LM-1/CowBell11.wav",
	"Samples/Percussion/LM-1/CowBell12.wav",
	"Samples/Percussion/LM-1/CowBell2.wav",
	"Samples/Percussion/LM-1/CowBell3.wav",
	"Samples/Percussion/LM-1/CowBell4.wav",
	"Samples/Percussion/LM-1/CowBell5.wav",
	"Samples/Percussion/LM-1/CowBell6.wav",
	"Samples/Percussion/LM-1/CowBell7.wav",
	"Samples/Percussion/LM-1/CowBell8.wav",
	"Samples/Percussion/LM-1/CowBell9.wav",
	"Samples/Percussion/LM-1/RimShot1.wav",
	"Samples/Percussion/LM-1/RimShot2.wav",
	"Samples/Percussion/LM-1/RimShot3.wav",
	"Samples/Percussion/LM-1/RimShot4.wav",
	"Samples/Percussion/LM-1/RimShot5.wav",
	"Samples/Percussion/LM-1/RimShot6.wav",
	"Samples/Percussion/LM-1/RimShot7.wav",
	"Samples/Percussion/LM-1/RimShot8.wav",
	"Samples/Percussion/LM-1/RimShot9.wav",
	"Samples/Percussion/LM-1/Tambourine1.wav",
	"Samples/Percussion/LM-1/Tambourine2.wav",
	"Samples/Percussion/LM-1/Tambourine3.wav",
	"Samples/Percussion/MasterRhythm/SM-8_RIMSHOT_01.wav",
	"Samples/Percussion/MasterRhythm/SM-8_RIMSHOT_02.wav",
	"Samples/Percussion/MasterRhythm/SM-8_RIMSHOT_03.wav",
	"Samples/Percussion/MasterRhythm/SM-8_RIMSHOT_04.wav",
	"Samples/Percussion/MasterRhythm/SM-8_RIMSHOT_05.wav",
	"Samples/Percussion/MasterRhythm/SM-8_RIMSHOT_06.wav",
	"Samples/Percussion/MasterRhythm/SM-8_RIMSHOT_07.wav",
	"Samples/Percussion/MasterRhythm/SM-8_RIMSHOT_08.wav",
	"Samples/Snare/DMX/06_SNARE_01.wav",
	"Samples/Snare/DMX/07_SNARE_02.wav",
	"Samples/Snare/DMX/08_SNARE_03.wav",
	"Samples/Snare/LM-1/Snare1.wav",
	"Samples/Snare/LM-1/Snare10.wav",
	"Samples/Snare/LM-1/Snare11.wav",
	"Samples/Snare/LM-1/Snare2.wav",
	"Samples/Snare/LM-1/Snare3.wav",
	"Samples/Snare/LM-1/Snare4.wav",
	"Samples/Snare/LM-1/Snare5.wav",
	"Samples/Snare/LM-1/Snare6.wav",
	"Samples/Snare/LM-1/Snare7.wav",
	"Samples/Snare/LM-1/Snare8.wav",
	"Samples/Snare/LM-1/Snare9.wav",
	"Samples/Snare/MasterRythm/SM-8_SNARE_01.wav",
	"Samples/Snare/MasterRythm/SM-8_SNARE_02.wav",
	"Samples/Snare/MasterRythm/SM-8_SNARE_03.wav",
	"Samples/Snare/MasterRythm/SM-8_SNARE_04.wav",
	"Samples/Snare/MasterRythm/SM-8_SNARE_05.wav",
	"Samples/Snare/MasterRythm/SM-8_SNARE_06.wav",
	"Samples/Snare/MasterRythm/SM-8_SNARE_07.wav",
	"Samples/Snare/MasterRythm/SM-8_SNARE_08.wav",
	"Samples/Snare/MasterRythm/SM-8_SNARE_09.wav",
	"Samples/Snare/MasterRythm/SM-8_SNARE_10.wav",
	"Samples/Snare/MasterRythm/SM-8_SNARE_11.wav",
	"Samples/Snare/MasterRythm/SM-8_SNARE_12.wav",
	"Samples/Snare/MasterRythm/SM-8_SNARE_13.wav",
	"Samples/Snare/MasterRythm/SM-8_SNARE_14.wav",
	"Samples/Snare/MasterRythm/SM-8_SNARE_15.wav",
	"Samples/Snare/MasterRythm/SM-8_SNARE_16.wav",
	"Samples/Toms/DMX/12_TOM_01.wav",
	"Samples/Toms/DMX/13_TOM_02.wav",
	"Samples/Toms/DMX/14_TOM_03.wav",
	"Samples/Toms/DMX/15_TOM_04.wav",
	"Samples/Toms/DMX/16_TOM_05.wav",
	"Samples/Toms/DMX/17_TOM_06.wav",
	"Samples/Toms/DX/DXLOWTOMSUPPLEMENTAL1.wav",
	"Samples/Toms/DX/DXLOWTOMSUPPLEMENTAL2.wav",
	"Samples/Toms/DX/DXLOWTOMSUPPLEMENTAL3.wav",
	"Samples/Toms/DX/DXLOWTOMSUPPLEMENTAL4.wav",
	"Samples/Toms/DX/DXLOWTOMSUPPLEMENTAL5.wav",
	"Samples/Toms/DX/DXLOWTOMSUPPLEMENTAL6.wav",
	"Samples/Toms/DX/DXLOWTOMSUPPLEMENTAL7.wav",
	"Samples/Toms/DX/DXLOWTOMSUPPLEMENTAL8.wav",
	"Samples/Toms/DX/DXMIDTOMSUPPLEMENTAL1.wav",
	"Samples/Toms/DX/DXMIDTOMSUPPLEMENTAL2.wav",
	"Samples/Toms/DX/DXMIDTOMSUPPLEMENTAL3.wav",
	"Samples/Toms/DX/DXMIDTOMSUPPLEMENTAL4.wav",
	"Samples/Toms/DX/DXMIDTOMSUPPLEMENTAL5.wav",
	"Samples/Toms/DX/DXMIDTOMSUPPLEMENTAL6.wav",
	"Samples/Toms/DX/DXMIDTOMSUPPLEMENTAL7.wav",
	"Samples/Toms/DX/DXMIDTOMSUPPLEMENTAL8.wav",
	"Samples/Long/07039111.wav",
	"Samples/Long/E03a-2006-05-09-1004utc-by_tING.wav"
	"Samples/BlackStar/Everything Center Long.snd",
	"Samples/BlackStar/Mics Center Speaker LR.snd",
	"Samples/BlackStar/Mics Center Speakers Center.snd",
	"Samples/BlackStar/Mics Center Speakers LR Bottom.snd",
	"Samples/BlackStar/Mics Center Speakers Mid Long Sweep.snd",
	"Samples/BlackStar/Mics High Speakers Low.snd",
	"Samples/BlackStar/Mics High Speakers Mid.snd",
	"Samples/BlackStar/Mics LR Speakers Mid.snd",
	"Samples/BlackStar/Mics Middle Speakers Back.snd",
	"Samples/BlackStar/Mics Spread Speaker Back.snd",
	"Samples/BlackStar/Mics Wide Speakers Wide.snd",
	"Samples/BlackStar/Speakers facing middle Mics LR.snd",
	"Samples/Sfx/boom.wav"
	};
	
	 
	unordered_map<string, shared_ptr<Sample>> sampleCache;

public:
	SampleManager() {
		;
	}

	vector<string>& getSampleList(){
		return sampleList;
	}


	//Indexed via getSampleList
	shared_ptr<Sample> getSample(const size_t idx_) {
			 
		if (sampleCache.find(sampleList[idx_]) == sampleCache.end()) {
			sampleCache[sampleList[idx_]] = make_shared<Sample>(sampleList[idx_]);
		} 
			
		return sampleCache[sampleList[idx_]];

	}

	//By name, we need a default empty sample because if the cache is empty this causes problems, or it's pain just not loading stuff properly
	shared_ptr<Sample> getSample(const string name_) {

		if (name_.size()) {
			if (sampleCache.find(name_) == sampleCache.end()) {
				sampleCache[name_] = make_shared<Sample>(name_);
			}

			return sampleCache[name_];
		}
		
		return nullptr;
	}

	string getName(const shared_ptr<Sample> sample) {
		for (auto& s : sampleCache) {
			if (s.second == sample) {
				return s.first;
			}
		}
		return string("");
	}

	void clear_cache() {
		sampleCache.clear();
	}



};

