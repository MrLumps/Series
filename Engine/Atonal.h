#pragma once
// Data driven header only music theory lib
// Be sure to define NOMINMAX for the preprocessor on windows
// Still not sure if I should set 0v to A or C
// Frankly C seems nutty to me

//Todo
//Make work with negative notes better
//Fix warnings and dubious crud
//Check chromaticseventh = operator
//Add inversion support
//Add other alternate voicing support


#include <memory>
#include <cstdint>
#include <array>
#include <iostream>
#include <vector>
#include <algorithm>

#include <math.h>

#define NOMINMAX

#ifdef ATONAL_NAMES
#include <string>
#endif // ATONAL_NAMES

#ifdef ATONAL_NAMES
//  enharmonic names removed
const std::array<std::string, 12> NoteNamesEN = {
	"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};
const std::array<std::string, 11> ScaleTypeNamesEN = {
	"Major", "Natural Minor", "Harmonic Minor", "Melodic Minor", "Ionian", "Dorian", "Phrygian", "Lydian", "Mixolydian", "Aeolian", "Locrian"
};
const std::array<std::string, 8> IntervalDegreeEN = {
	"First", "Second", "Third", "Fourth", "Fifth", "Sixth", "Seventh", "Eighth"
};
const std::array<std::string, 5> IntervalTypeEN = {
	"Diminished", "Minor", "Perfect", "Major", "Augmented"
};
const std::array<std::string, 4> TriadTypeEN = {
	"Major", "Minor", "Augmented", "Diminished"
};
std::array<std::string, 8> SeventhTypeEN = {
	"Diminished", "Half Diminished","Minor", "Minor Major", "Dominant", "Major", "Augmented", "Augmented Major"
};
const std::array<std::string, 5> SlendroNamesEN = {
	"siji", "loro", "telu", "lima", "enem"
};
const std::array<std::string, 7> PelogNamesEN = {
	"siji", "loro", "telu", "papat", "lima", "enem", "pitu"
};


const std::array<std::string, 18> BaseScaleNamesEN = {
	"Equal Temperment","Quarter Common Meantone","Pythagorean D","Just Intonation","Pythagorean C","Guqin","7 Shrutis","22 Shrutis","Slendro","Pelog Selisir","Pelog Tembung","Pelog Sunaren","Pelog Baro","Pelog Lebeng","Slendro gede","Slendro alit","Pengenter gede","Pengenter alit"
};

#endif // ATONAL_NAMES


// Todo
// update chromaticinterval to include note changes
//  currently works but could be more better

//  Chromatic scale note names
enum class ChromaticNotes {
	C,
	Cs,
	Db = Cs,
	D,
	Ds,
	Eb = Ds,
	E,
	Fb = E,
	F,
	Es = F,
	Fs,
	Gb = Fs,
	G,
	Gs,
	Ab = Gs,
	A,
	As,
	Bb = As,
	B,
	Bs = C,
	Cb = B
};



enum class SlendroNotes {
	ji,
	ro,
	lu,
	ma,
	nem
};


enum class PelogNotes {
	ji,
	ro,
	lu,
	pat,
	ma,
	nem,
	pi
};

//Note pianoOctave[12] = {
//	{ int(ChromaticNotes::C), 0 },
//	{ int(ChromaticNotes::Cs), 0 },
//	{ int(ChromaticNotes::D), 0 },
//	{ int(ChromaticNotes::Ds), 0 },
//	{ int(ChromaticNotes::E), 0 },
//	{ int(ChromaticNotes::F), 0 },
//	{ int(ChromaticNotes::Fs), 0 },
//	{ int(ChromaticNotes::G), 0 },
//	{ int(ChromaticNotes::Gs), 0 },
//	{ int(ChromaticNotes::A), 1 },
//	{ int(ChromaticNotes::As), 1 },
//	{ int(ChromaticNotes::B), 1 },
//};






// Notes and BaseOctave have the following relationship
// A note is just a whole number from 0 to whatever
// Each note represents a whole number of semitones
// An octave is some number of divisions in a doubling of frequency which can be divided up many ways
// BaseOctave provides an index for tuningTable which contains octave sets of many sizes

//
// The chromatic structs will only really work with BaseOctaves of 12
//

// A Note is simply an integer number of semitones
struct Note {

	int n = 0; // Midi goes up to 127 but you may want more if you go with longer tunings
						 //int octave = 0; // meh

	Note(int n) : n(n) {}
	Note(ChromaticNotes n) : n(static_cast<int>(n)) {}

	Note() { n = 0; }


	//Octave and relative note values (chromatic scale, eg 1-12) and operations

	//Current octave
	const int octave() {
		return n / 12;
	}

	//Set octave, 0-8ish
	void octave(const int octave_) {
		n += (octave_ - octave()) * 12;
	}

	//Current relative note
	const int relative() {
		return n % 12;
	}

	//Set relative note
	void relative(const int n_) {
		n = n_ + (octave() * 12);
	}

	//Shift the current note s_ steps
	void shift(int s_) {
		s_ += relative();

		if (s_ < 0) {
			s_ += 12;
		}

		n = (s_ % 12) + (octave() * 12);
	}

	//Midi assumes a western chromatic scale or 12-ET
	//Midi also does not do negative notes
	const int midi() const {
		if (n > 0) {
			return(n);
		}
		else {
			return 0;
		}
	}

	void fromMidi(const int midi) {
		n = midi;
	}

	Note& operator+=(const int& semitones) {
		n += semitones;
		return *this;
	}

	friend Note operator+(Note l, const int& semitones) {
		l += semitones;
		return l;
	}

	Note& operator++() {
		n++;
		return *this;
	}

	Note& operator--() {
		n--;
		return *this;
	}

	//copy assignment
	Note& operator=(const Note& other) {
		if (this != &other) {
			n = other.n;
		}
		return *this;
	}

	friend inline bool operator< (const Note& l, const Note& r) {
		if (l.n < r.n) {
			return true;
		}
		return false;
	}

	friend inline bool operator> (const Note& l, const Note& r) { return r < l; }
	friend inline bool operator<=(const Note& l, const Note& r) { return !(l > r); }
	friend inline bool operator>=(const Note& l, const Note& r) { return !(l < r); }


};


enum class TuningType {
	EQUAL_TEMPERED,
	QUARTER_COMMA_MEANTONE,
	PYTHAGOREAN_D,
	JUST_INTONATION,
	PYTHAGOREAN_C,
	GUQIN,
	SHRUTIS_7,
	SHRUTIS_22,
	RAJA_FOLK,
	SLENDRO,
	PELOG_SELISIR,
	PELOG_TEMBUNG,
	PELOG_SUNAREN,
	PELOG_BARO,
	PELOG_LEBENG,
	SLENDRO_GEDE,
	SLENDRO_ALIT,
	PENGENTER_GEDE,
	PENGENTER_ALIT,
	GAMELANDEGUNG,
	PERSIAN_SEHTAR,
	KOTO,
	NUM_TYPES
};

struct Cents {
	int size;
	TuningType tuning;
	std::vector<float> cents;
};

const Cents tuningTable[] = {
	//Every semitone is 100 cents by definition
	{ 12, TuningType::EQUAL_TEMPERED,{ 0.0f, 100.0f, 200.0f, 300.0f, 400.0f, 500.0f, 600.0f, 700.0f, 800.0f, 900.0f, 1000.0f, 1100.0f } },
	{ 12, TuningType::QUARTER_COMMA_MEANTONE,{ 0.0f,	76.0f, 193.2f, 310.3f, 386.3f, 503.4f, 579.5f, 696.6f, 772.6f, 889.7f, 1006.8f,	1082.9f } },
	//Pythagorean D 1:1, 256:243, 9:8, 32:27, 81:64, 4:3, 729:512, 3:2, 128:81, 27:16, 16:9, 243:128, 2:1	
	{ 12, TuningType::PYTHAGOREAN_D,{ 0.0f, 90.0f, 204.0f, 294.0f, 408.0f, 498.0f, 612.0f, 702.0f, 792.0f, 906.0f, 996.0f, 1110.0f } },
	//Just intonation       1:1, 9:8,   5:4,   4:3,   3:2,   5:3,   15:8, 2:1
	{ 7,  TuningType::JUST_INTONATION,{ 0.0f, 204.0f, 386.0f, 498.0f, 702.0f, 884.0f, 1088.0f } },
	//Pythagorean C 1:1	9:8	81:64	4:3	3:2	27:16	243:128	2:1
	{ 7,  TuningType::PYTHAGOREAN_C,{ 0.0f,	150.0f, 318.75f, 400.0f, 600.0f, 825.0f,	1078.125f } },
	//Guqin 1⁄8, ​1⁄6, ​1⁄5, ​1⁄4, ​1⁄3, ​2⁄5, ​1⁄2, ​3⁄5, ​2⁄3, ​3⁄4, ​4⁄5, ​5⁄6, ​7⁄8, 2
	{ 13, TuningType::GUQIN,{ 150.0f,200.0f,240.0f,300.0f,400.0f,480.0f,600.0f,720.0f,800.0f,900.0f,960.0f,1000.0f,1050.0f } }, // Probably wrong
																																																																					//7 Shrutis
	{ 7,  TuningType::SHRUTIS_7,{ 0.0f, 204.0f, 386.0f, 498.0f, 702.0f, 906.0f, 1088.0f } },
	//22 Shrutis
	{ 22, TuningType::SHRUTIS_22,{ 0.0f, 90.0f, 112.0f, 182.0f, 204.0f, 294.0f, 316.0f, 386.0f, 408.0f, 498.0f, 520.0f, 590.0f, 612.0f, 702.0f, 792.0f, 814.0f, 884.0f, 906.0f, 996.0f, 1018.0f, 1110.0f } },
	{ 6,  TuningType::RAJA_FOLK,{ 0.0f, 203.910002f, 386.313714f, 498.044999f, 701.955001f, 1088.268715f } },
	//Slendro =~ 5-ET
	{ 5,  TuningType::SLENDRO,{ 0.0f, 240.0f, 480.0f, 720.0f, 960.0f } },
	//Pelog =~ 9-ET
	//http://www.neuroscience-of-music.se/pelog_main.htm
	{ 5,  TuningType::PELOG_SELISIR,{ 0.0f, 133.33f, 266.66f, 666.65f, 799.98f } },
	{ 5,  TuningType::PELOG_TEMBUNG,{ 0.0f, 133.33f,533.32f,666.65f,799.98f } },
	{ 5,  TuningType::PELOG_SUNAREN,{ 133.33f,266.66f,666.65f,799.98f,933.31f } },
	{ 5,  TuningType::PELOG_BARO,{ 0.0f,266.66f,533.32f,666.65f,933.31f } },
	{ 7,  TuningType::PELOG_LEBENG,{ 0.0f,133.33f,266.66f,533.32f,666.65f,799.98f,933.31f } },
	{ 5,  TuningType::SLENDRO_GEDE,{ 133.33f,266.66f,533.32f,799.98f,933.31f } },
	{ 5,  TuningType::SLENDRO_ALIT,{ 0.0f,266.66f,533.32f,666.65f,933.31f } },
	{ 5,  TuningType::PENGENTER_GEDE,{ 0.0f,133.33f,533.32f,666.65f,933.31f } },
	{ 5,  TuningType::PENGENTER_ALIT,{ 0.0f,266.66f, 533.32f,799.98f,933.31f } },
	//Gamelan Degung, Kabupaten Sukabumi. 1/1=363 Hz  
	{ 5,  TuningType::GAMELANDEGUNG,{ 0.0f, 155.003f, 344.805f, 693.988f, 823.199f } },
	//Hormoz Farhat, average of observed Persian tar and sehtar tunings (1966)
	{ 17, TuningType::PERSIAN_SEHTAR,{ 0.0f, 90.0f, 135.0f, 205.0f, 295.0f, 340.0f, 410.0f, 500.0f, 565.0f, 630.0f, 700.0f, 790.0f, 835.0f, 905.0f, 995.0f, 1040.0f, 1110.0f } },
	//Observed Japanese pentatonic koto scale. Helmholtz/Ellis p.519, nr.112
	{ 5,  TuningType::KOTO,{ 0.0f, 185.0f, 337.0f, 683.0f, .199f } },
};



struct BaseOctave {
		const float oneCent = 1.0f / 1200.0f;
		float base_frequency = 8.1757989156f; // C0 in hz A = 440
		TuningType type = TuningType::EQUAL_TEMPERED;

		const size_t size() const {
			return(tuningTable[static_cast<int>(type)].size);
		}

		//+/- 8192 for a range of 2 semitones or 200 cents
		//const float midiPitchBend() const {
		//	const int index = n % tuningTable[static_cast<int>(type)].size;
		//	const int chromatic_i = n % 12;
		//	const auto diffInCents = tuningTable[static_cast<int>(BaseOctave::TuningType::EQUAL_TEMPERED)].cents[chromatic_i] - tuningTable[static_cast<int>(type)].cents[index];
		//	return(diffInCents * 40.96);
		//}

		//int midiNFromFreq(const float freq) {
		//	const float L2 = log(2.0);
		//	const float L440 = log(base_frequency);
		//	return(static_cast<int>(int(round(12 * (log(freq) - L440) / L2 + 69.0)) % 12));
		//}

		inline float clamp(const float val_, const float min_, const float max_) {
			return fmaxf(fminf(val_, fmaxf(min_, max_)), fminf(min_, max_));
		}


		//This is fine for midi, but doesn't support negative notes yet
		//Update with http://pages.mtu.edu/~suits/NoteFreqCalcs.html
		const float freq(const Note n_) const {
			const int index = n_.n % (int)tuningTable[static_cast<int>(type)].size;
			const int octave = abs(n_.n) / (int)tuningTable[static_cast<int>(type)].size;
			return base_frequency * powf(2.0f, static_cast<float>(octave)) * powf(2.0f, tuningTable[static_cast<int>(type)].cents[index] / 1200.0f);
		}

		const float cv(const Note n_) const {
			const int index = abs(n_.n) % (int)tuningTable[static_cast<int>(type)].size;
			const int octave = abs(n_.n) / (int)tuningTable[static_cast<int>(type)].size;
			return static_cast<float>(octave) + (tuningTable[static_cast<int>(type)].cents[index] * oneCent);
		}

		const float cv(const int n_) const {
			const int index = abs(n_) % (int)tuningTable[static_cast<int>(type)].size;
			const int octave = abs(n_) / (int)tuningTable[static_cast<int>(type)].size;
			return static_cast<float>(octave) + (tuningTable[static_cast<int>(type)].cents[index] * oneCent);
		}


		//3 CV quantize functions
		//low 0 - 1
		//bi -5 - 5
		//hi 0 - 10
		Note fromCvLow(const float signal_, const int octaveSpread_) {
			return Note((int)(clamp(signal_, 0.0f, 1.0f) * (tuningTable[static_cast<int>(type)].size + (tuningTable[static_cast<int>(type)].size * octaveSpread_))));
		}

		Note fromCvBi(const float signal_, const int octaveSpread_) {
			return Note((int)(clamp(signal_, -5.0f, 5.0f) / 5.0f) * ((int)tuningTable[static_cast<int>(type)].size + (tuningTable[static_cast<int>(type)].size * octaveSpread_)));
		}

		Note fromCvHi(const float signal_, const int octaveSpread_) {
			return Note((int)(clamp(signal_, 0.0f, 10.0f) / 10.0f) * ((int)tuningTable[static_cast<int>(type)].size + (tuningTable[static_cast<int>(type)].size * octaveSpread_)));
		}
		
		void setTuningFreq(const float hz_) {
			//Range checking here?
			base_frequency = hz_;
		}

		void setTuningType(const TuningType type_) {
			type = type_;
		}

};











// Generic intervals in semitones
struct Interval {
public:

	int semitones;

public:
	inline const  bool isValid() const {
		return (static_cast<bool>(semitones));
	}

	const int steps() const {
		return semitones;
	}

	const Note get(const Note start) const {
		return start + steps();
	}
#ifdef ATONAL_NAMES		
	const std::string name() const {
		return std::to_string(semitones);
	}
#endif	

	//possibly 2 notes could have different tunings
	//Just use the 1st note's tuning
	void fromNotes(const Note low, const Note high) {
		semitones = high.n - low.n;
	}

};





//Western Chromatic scale musical structures
struct ChromaticScale {
	enum ScaleTypes {
		MAJOR,
		NATURAL_MINOR,
		HARMONIC_MINOR,
		MELODIC_MINOR,
		IONIAN,
		DORIAN,
		PHRYGIAN,
		LYDIAN,
		MIXOLYDIAN,
		AEOLIAN,
		LOCRIAN,
		SCALE_TYPES
	};

	Note tonic;
	ScaleTypes scale = ScaleTypes::MAJOR;

	ChromaticScale() { ; }
	ChromaticScale(Note tonic, ScaleTypes scale) : tonic(tonic), scale(scale) { ; }

	// The number of semitones required to find the next note in the scale
	const std::array<std::array<int, 7>, ChromaticScale::ScaleTypes::SCALE_TYPES> ScaleSteps = { {
		{ 0,2,4,5,7,9,11 },  // Major WWhWWWh
		{ 0,2,3,5,7,8,10 },  // Natural Minor  WhWWhWW
		{ 0,2,3,5,7,8,11 },  // Harmonic Minor WhWWhW+W
		{ 0,2,3,5,7,9,11 },  // Melodic Minor  WhWWh+W+W
		{ 0,2,4,5,7,9,11 },  // Ionian WWhWWWh (major)
		{ 0,2,3,5,7,9,10 },  // Dorian WhWWWhW
		{ 0,1,3,5,7,8,10 },  // Phrygian hWWWhWW
		{ 0,2,4,6,7,9,11 },  // Lydian WWWhWWh
		{ 0,2,4,5,7,9,10 },  // Mixoldian WWhWWhW
		{ 0,2,3,5,7,8,10 },  // Aeolian WhWWhWW (natural minor)
		{ 0,1,3,5,6,8,10 }   // Locrian hWWhWWW
		} };

	const inline Note operator[](const int idx) const {
		Note scaleNote = tonic + ScaleSteps[scale][idx % ScaleSteps[scale].size()];
		return scaleNote;
	}

	const inline Note getNote(const int idx) const {
		Note scaleNote = tonic + ScaleSteps[scale][idx % ScaleSteps[scale].size()];
		return scaleNote;
	}

#ifdef ATONAL_NAMES
	const std::string name() const {
		return tonic.name() + " " + ScaleTypeNamesEN[static_cast<int>(scale)];
	}
#endif	
};

//
//Note
//Diatonics use relative positions in scales
//The Chromatic intervals triads and sevenths use absolute values
//


struct DiatonicTriad : ChromaticScale {

public:

	const inline Note getTriad(const int n_, const int t_) {
		switch (t_) {
		case 1:
			return getNote(n_ + 2);
		case 2:
			return getNote(n_ + 4);
		default:
			return getNote(n_);
		}
	}

	const size_t size() const {
		return 3;
	}

#ifdef ATONAL_NAMES
//	const std::string name() const {
//		return root.name() + " " + DiatonicTriadTypeEN[static_cast<int>(type)];
//	}
#endif

};


struct DiatonicSeventh : ChromaticScale {

public:

	const inline Note getSeventh(const int root_, const int n_) {
		switch (n_) {
		case 1:
			return getNote(root_ + 2);
		case 2:
			return getNote(root_ + 4);
		case 3:
			return getNote(root_ + 6);
		default:
			return getNote(root_);
		}
	}

	const size_t size() const {
		return 4;
	}

#ifdef ATONAL_NAMES
//	const std::string name() const {
//		return root.name() + " " + DiatonicSeventhTypeEN[static_cast<int>(type)];
//	}
#endif

};


struct DiatonicNinth : ChromaticScale {

public:

	const inline Note getSeventh(const int root_, const int n_) {
		switch (n_) {
		case 1:
			return getNote(root_ + 2);
		case 2:
			return getNote(root_ + 4);
		case 3:
			return getNote(root_ + 6);
		case 4:
			return getNote(root_ + 8);
		default:
			return getNote(root_);
		}
	}

	const size_t size() const {
		return 4;
	}

#ifdef ATONAL_NAMES
	//	const std::string name() const {
	//		return root.name() + " " + DiatonicSeventhTypeEN[static_cast<int>(type)];
	//	}
#endif

};


struct ChromaticInterval {
public:
	enum class Degrees {
		FIRST,
		SECOND,
		THIRD,
		FOURTH,
		FIFTH,
		SIXTH,
		SEVENTH,
		EIGHTH,
		NINTH,
		INTERVAL_DEGREES
	};

	enum class Types {
		DIMINISHED,
		MINOR,
		PERFECT,
		MAJOR,
		AUGMENTED,
		INTERVAL_TYPES
	};

	Types type;
	Degrees degree;

	ChromaticInterval(Types type, Degrees degree) : type(type), degree(degree) { ; }
	ChromaticInterval() { type = Types::DIMINISHED; degree = Degrees::FIRST; }

private:
	const int IntervalSteps[static_cast<int>(Degrees::INTERVAL_DEGREES)][static_cast<int>(Types::INTERVAL_TYPES)] = {
		//const int IntervalSteps[8][5] = {
		{ -1, -1,  0, -1,  1 },
		{ 0,  1, -1,  2,  3 }, 
		{ 2,  3, -1,  4,  5 }, 
		{ 4, -1,  5, -1,  6 }, 
		{ 6, -1,  7, -1,  8 }, 
		{ 7,  8, -1,  9, 10 },
		{ 9, 10, -1, 11, 12 },
		{ 11, -1, 12, -1, 13 },
		{ 12, 13, 11, 14, 15 }, //Octave + a second
	};

public:
	inline const  bool isValid() const {
		return (IntervalSteps[static_cast<int>(degree)][static_cast<int>(type)] < 0 ? false : true);
	}

	const int steps() const {
		// I guess since it's all strongly typed this should not be required
		if (degree >= Degrees::INTERVAL_DEGREES || type >= Types::INTERVAL_TYPES || degree < Degrees::FIRST || type < Types::DIMINISHED) {
			return (IntervalSteps[0][0]);
		}
		return (IntervalSteps[static_cast<int>(degree)][static_cast<int>(type)]);
	}

	const Note get(const Note start) const {
		return start + steps();
	}
#ifdef ATONAL_NAMES		
	const std::string name() const {
		return IntervalTypeEN[static_cast<int>(type)] + " " + IntervalDegreeEN[static_cast<int>(degree)];
	}
#endif	
	//  Increments to the next valid degree then wraps around
	void nextDegree() {
		for (int d = static_cast<int>(degree) + 1; d < static_cast<int>(Degrees::INTERVAL_DEGREES); d++) {
			if (IntervalSteps[d][static_cast<int>(type)] > 0) {
				degree = static_cast<Degrees>(d);
				return;
			}
		}

		for (int d = 0; d < static_cast<int>(degree); d++) {
			if (IntervalSteps[d][static_cast<int>(type)] > 0) {
				degree = static_cast<Degrees>(d);
				return;
			}
		}

		degree = static_cast<Degrees>(0);

	}

	//  Increments to the next valid type then wraps around
	void nextType() {
		for (int t = static_cast<int>(type) + 1; t < static_cast<int>(Types::INTERVAL_TYPES); t++) {
			if (IntervalSteps[static_cast<int>(degree)][t] > 0) {
				type = static_cast<Types>(t);
				return;
			}
		}

		for (int t = 0; t < static_cast<int>(type); t++) {
			if (IntervalSteps[static_cast<int>(degree)][t] > 0) {
				type = static_cast<Types>(t);
				return;
			}
		}

		type = static_cast<Types>(0);
	}

	//Hmmm perhaps I need a function that will give me the interval between 2 notes
	//So I can do inversions
	// That would look something like
	//Note C4 = { Notes::C, 4 };
	//Interval pfth = { Degrees::FIFTH, Types::Perfect };
	//Note NewNote = C4 + pfth.steps(); // G
	//Interval Hmmm = pfth.fromnotes(low, high) // hmmm
	//Interval Inverted = pfth.invert(low, high) // hmmm


	//Rather then binary s
	//    0   1   2   3   4
	//0 {-1, -1,  0, -1,  1 }, First
	//1 { 0,  1, -1,  2,  3 }, second
	//2 { 2,  3, -1,  4,  5 }, third
	//3 { 4, -1,  5, -1,  6 }, fourth
	//4 { 6, -1,  7, -1,  8 }, fifth
	//5 { 7,  8, -1,  9, 10 }, sixth
	//6 { 9, 10, -1, 11, 12 }, seventh
	//7 { 11, -1, 12, -1, 13 } eighth
	//    dim min per maj aug


	//This is just wrong... fix
	//
	//void fromNotes(const Note low, const Note high) {
	//	const int semitones = abs(high.n - low.n);
	//	const ChromaticInterval lookuptable[14] = {
	//		{ Types::PERFECT,   Degrees::FIRST },
	//		{ Types::MINOR,     Degrees::SECOND },
	//		{ Types::MAJOR,     Degrees::SECOND },
	//		{ Types::MINOR,     Degrees::THIRD },
	//		{ Types::MAJOR,     Degrees::THIRD },
	//		{ Types::PERFECT,   Degrees::FOURTH },
	//		{ Types::AUGMENTED, Degrees::FOURTH },
	//		{ Types::PERFECT,   Degrees::FIFTH },
	//		{ Types::MINOR,     Degrees::SIXTH },
	//		{ Types::MAJOR,     Degrees::SIXTH },
	//		{ Types::MINOR,     Degrees::SEVENTH },
	//		{ Types::MAJOR,     Degrees::SEVENTH },
	//		{ Types::PERFECT,   Degrees::EIGHTH },
	//		{ Types::AUGMENTED, Degrees::EIGHTH }
	//	};
	//
	//	if (abs(semitones) > 14) {
	//		degree = static_cast<Degrees>(0);
	//		type = static_cast<Types>(0);
	//	}
	//	else {
	//		degree = lookuptable[abs(semitones)].degree;
	//		type = lookuptable[abs(semitones)].type;
	//	}
	//
	//}

};




struct ChromaticTriad {
public:
	enum Types {
		MAJOR,
		MINOR,
		AUGMENTED,
		DIMINISHED,
		SUSPENDED2,
		SUSPENDED4,
		TRIAD_TYPES
	};

	enum Inversions {
		NONE,
		FIRST,
		SECOND,
		THIRD,
	};

	Note root;
	Types triad;
	Inversions inversion; //Not implemented here yet
	
	ChromaticTriad(Note root, Types type, Inversions inversion) : root(root), triad(type), inversion(inversion) { ; }
	ChromaticTriad(Note root, Types type) : root(root), triad(type) { inversion = Inversions::NONE; }
	ChromaticTriad(Note root) : root(root) { triad = Types::MAJOR; inversion = Inversions::NONE; }
	ChromaticTriad() { root.n = int(ChromaticNotes::C);  triad = Types::MAJOR; inversion = Inversions::NONE; }
	

private:
	const std::array<std::array<ChromaticInterval, 2>, Types::TRIAD_TYPES> TriadIntervals = { {
		{ { { ChromaticInterval::Types::MAJOR, ChromaticInterval::Degrees::THIRD },{ ChromaticInterval::Types::PERFECT,    ChromaticInterval::Degrees::FIFTH } } }, //   Major
		{ { { ChromaticInterval::Types::MINOR, ChromaticInterval::Degrees::THIRD },{ ChromaticInterval::Types::PERFECT,    ChromaticInterval::Degrees::FIFTH } } }, //   Minor
		{ { { ChromaticInterval::Types::MAJOR, ChromaticInterval::Degrees::THIRD },{ ChromaticInterval::Types::AUGMENTED,  ChromaticInterval::Degrees::FIFTH } } }, //   Augmented
		{ { { ChromaticInterval::Types::MINOR, ChromaticInterval::Degrees::THIRD },{ ChromaticInterval::Types::DIMINISHED, ChromaticInterval::Degrees::FIFTH } } }, //   Diminished
		{ { { ChromaticInterval::Types::MAJOR, ChromaticInterval::Degrees::SECOND },{ ChromaticInterval::Types::PERFECT,   ChromaticInterval::Degrees::FIFTH } } }, //   Suspended 2
		{ { { ChromaticInterval::Types::PERFECT, ChromaticInterval::Degrees::FOURTH },{ ChromaticInterval::Types::PERFECT, ChromaticInterval::Degrees::FIFTH } } }  //   Suspended 4
		} };

public:

	//If inversion is 0 do nothing
	//1 root note +12
	//2 root and 1 +12
	//3 all +12
	const Note operator[](const int idx) const {
		switch (idx) {
		case 1:
			return TriadIntervals[triad][0].get(root);
		case 2:
			return TriadIntervals[triad][1].get(root);
		default:
			return root;
		}
	}

	const size_t size() const {
		return 3;
	}

#ifdef ATONAL_NAMES
	const std::string name() const {
		return root.name() + " " + TriadTypeEN[static_cast<int>(triad)];
	}
#endif
};



//Triad structure for Riemannian transformations
//Since midi doesn't cope with negative notes
//some care should be taken with the base_
//Thanks to noise engineering for their helpful reading list and documentation
//that inspired most of this
struct RiemannianTriad {
public:

	//One per ChromaticTriad type
	enum Pairs {
		MAJOR_MINOR,
		MINOR_MAJOR,
		AUGMENTED_MAJOR,
		DIMINISHED_MINOR,
		SUSPENDED2_MAJOR,
		SUSPENDED4_MINOR,
		TRIAD_TYPES
	};
private:
	Pairs pair;
	int sign;

public:

	ChromaticTriad triad;


	RiemannianTriad(const ChromaticTriad base_) {
		setRoot(base_);
	}

	void setRoot(const ChromaticTriad base_) {
		triad.root = base_.root;
		triad.triad = base_.triad;

		pair = static_cast<Pairs>(static_cast<int>(base_.triad));
		sign = 1;
	}

	//Perhaps I should change this to a lookup table, seems branchy
	void flip() {
		sign = -sign;
		switch (pair) {
		case MAJOR_MINOR:
		case MINOR_MAJOR:
			if (triad.triad == ChromaticTriad::Types::MAJOR) {
				triad.triad = ChromaticTriad::Types::MINOR;
			}
			else if (triad.triad == ChromaticTriad::Types::MINOR) {
				triad.triad = ChromaticTriad::Types::MAJOR;
			}
			break;
		case AUGMENTED_MAJOR:
			if (triad.triad == ChromaticTriad::Types::MAJOR) {
				triad.triad = ChromaticTriad::Types::AUGMENTED;
			}
			else if (triad.triad == ChromaticTriad::Types::AUGMENTED) {
				triad.triad = ChromaticTriad::Types::MAJOR;
			}
			break;
		case DIMINISHED_MINOR:
			if (triad.triad == ChromaticTriad::Types::DIMINISHED) {
				triad.triad = ChromaticTriad::Types::MINOR;
			}
			else if (triad.triad == ChromaticTriad::Types::MINOR) {
				triad.triad = ChromaticTriad::Types::DIMINISHED;
			}
			break;
		case SUSPENDED2_MAJOR:
			if (triad.triad == ChromaticTriad::Types::MAJOR) {
				triad.triad = ChromaticTriad::Types::SUSPENDED2;
			}
			else if (triad.triad == ChromaticTriad::Types::SUSPENDED2) {
				triad.triad = ChromaticTriad::Types::MAJOR;
			}
			break;
		case SUSPENDED4_MINOR:
			if (triad.triad == ChromaticTriad::Types::SUSPENDED4) {
				triad.triad = ChromaticTriad::Types::MINOR;
			}
			else if (triad.triad == ChromaticTriad::Types::MINOR) {
				triad.triad = ChromaticTriad::Types::SUSPENDED4;
			}
			break;
		}
	}

	//0
	void P() {
		flip();
	}

	//Not sure if 4 and 9 will do the same sort of jobs with pairs other then major/minor
	//9
	void R() {
		triad.root.shift(sign * 9);
		flip();
	}

	//4
	void L() {
		triad.root.shift(sign * 4);
		flip();
	}

	void N() {
		R();
		L();
		P();
	}

	void S() {
		L();
		P();
		R();
	}

	void H() {
		L();
		P();
		L();
	}

};


struct ChromaticSeventh {
public:
	enum Types {
		DIMINISHED,
		HALF_DIMINISHED,
		MINOR,
		MINOR_MAJOR,
		DOMINANT,
		MAJOR,
		AUGMENTED,
		AUGMENTED_MAJOR,
		SUSPENDED2,
		SUSPENDED4,
		SEVENTH_TYPES
	};

	Note root;
	Types seventh;

	ChromaticSeventh(Note root, Types type) : root(root), seventh(type) { ; }
	ChromaticSeventh() { root.n = int(ChromaticNotes::C);  seventh = Types::DIMINISHED; }

private:
	const std::array<std::array<ChromaticInterval, 3>, Types::SEVENTH_TYPES> SeventhIntervals = { {
		{ { { ChromaticInterval::Types::MINOR, ChromaticInterval::Degrees::THIRD },{ ChromaticInterval::Types::DIMINISHED, ChromaticInterval::Degrees::FIFTH },{ ChromaticInterval::Types::DIMINISHED, ChromaticInterval::Degrees::SEVENTH } } }, //Dim
		{ { { ChromaticInterval::Types::MINOR, ChromaticInterval::Degrees::THIRD },{ ChromaticInterval::Types::DIMINISHED, ChromaticInterval::Degrees::FIFTH },{ ChromaticInterval::Types::MINOR,      ChromaticInterval::Degrees::SEVENTH } } }, //Half dim
		{ { { ChromaticInterval::Types::MINOR, ChromaticInterval::Degrees::THIRD },{ ChromaticInterval::Types::PERFECT,    ChromaticInterval::Degrees::FIFTH },{ ChromaticInterval::Types::MINOR,      ChromaticInterval::Degrees::SEVENTH } } }, //Minor
		{ { { ChromaticInterval::Types::MINOR, ChromaticInterval::Degrees::THIRD },{ ChromaticInterval::Types::PERFECT,    ChromaticInterval::Degrees::FIFTH },{ ChromaticInterval::Types::MAJOR,      ChromaticInterval::Degrees::SEVENTH } } }, //Minor major
		{ { { ChromaticInterval::Types::MAJOR, ChromaticInterval::Degrees::THIRD },{ ChromaticInterval::Types::PERFECT,    ChromaticInterval::Degrees::FIFTH },{ ChromaticInterval::Types::MINOR,      ChromaticInterval::Degrees::SEVENTH } } },//Dom
		{ { { ChromaticInterval::Types::MAJOR, ChromaticInterval::Degrees::THIRD },{ ChromaticInterval::Types::PERFECT,    ChromaticInterval::Degrees::FIFTH },{ ChromaticInterval::Types::MAJOR,      ChromaticInterval::Degrees::SEVENTH } } }, //Major
		{ { { ChromaticInterval::Types::MAJOR, ChromaticInterval::Degrees::THIRD },{ ChromaticInterval::Types::AUGMENTED,  ChromaticInterval::Degrees::FIFTH },{ ChromaticInterval::Types::MINOR,      ChromaticInterval::Degrees::SEVENTH } } }, //Aug
		{ { { ChromaticInterval::Types::MAJOR, ChromaticInterval::Degrees::THIRD },{ ChromaticInterval::Types::AUGMENTED,  ChromaticInterval::Degrees::FIFTH },{ ChromaticInterval::Types::MAJOR,      ChromaticInterval::Degrees::SEVENTH } } }, //Aug major
		{ { { ChromaticInterval::Types::MAJOR, ChromaticInterval::Degrees::SECOND },{ ChromaticInterval::Types::PERFECT,   ChromaticInterval::Degrees::FIFTH },{ ChromaticInterval::Types::MINOR,      ChromaticInterval::Degrees::SEVENTH } } }, //sus2
		{ { { ChromaticInterval::Types::MAJOR, ChromaticInterval::Degrees::FOURTH },{ ChromaticInterval::Types::PERFECT,   ChromaticInterval::Degrees::FIFTH },{ ChromaticInterval::Types::MINOR,      ChromaticInterval::Degrees::SEVENTH } } }  //sus4
		} };

public:
	const Note operator[](const int idx) const {
		switch (idx) {
		case 1:
			return SeventhIntervals[seventh][0].get(root);
		case 2:
			return SeventhIntervals[seventh][1].get(root);
		case 3:
			return SeventhIntervals[seventh][2].get(root);
		default:
			return root;
		}
	}

	void operator=(const ChromaticSeventh s_) {
		root = s_.root;
		seventh = s_.seventh;
	}




	const size_t size() const {
		return 4;
	}

#ifdef ATONAL_NAMES
	const std::string name() const {
		return root.name() + " " + SeventhTypeEN[static_cast<int>(seventh)];
	}
#endif
};


struct ChromaticNinth {
public:
	enum Types {
		DOMINANT_MINOR,
		DOMINANT_MAJOR,
		MINOR,
		MAJOR,
		SIXNINE_MINOR,
		SIXNINE_MAJOR,
		NINTH_TYPES
	};

	Note root;
	Types ninth;

	ChromaticNinth(Note root, Types type) : root(root), ninth(type) { ; }
	ChromaticNinth() { root.n = int(ChromaticNotes::C);  ninth = Types::DOMINANT_MINOR; }
	//dominant minor ninth chord consists of a dominant seventh chord and a minor ninth
	//dominant major ninth chord consists of a dominant seventh chord and a major ninth
	//minor ninth chord consist of a minor seventh chord and a major ninth
	//major ninth chord consist of a major seventh chord and a major ninth
	//the sixnine chord is a pentad with a major triad extended by a sixth and ninth above the root
	//minor 6/9 chord is a minor triad with an added 6th of the Dorian Mode and an added 9th
	//But 6th of dorian is +9 semitones which is a major seventh

private:
	const std::array<std::array<ChromaticInterval, 4>, Types::NINTH_TYPES> NinthIntervals = { {
		{ { { ChromaticInterval::Types::MAJOR, ChromaticInterval::Degrees::THIRD },{ ChromaticInterval::Types::PERFECT, ChromaticInterval::Degrees::FIFTH },{ ChromaticInterval::Types::MINOR, ChromaticInterval::Degrees::SEVENTH }, { ChromaticInterval::Types::MINOR, ChromaticInterval::Degrees::NINTH } }  }, 
		{ { { ChromaticInterval::Types::MAJOR, ChromaticInterval::Degrees::THIRD },{ ChromaticInterval::Types::PERFECT, ChromaticInterval::Degrees::FIFTH },{ ChromaticInterval::Types::MINOR, ChromaticInterval::Degrees::SEVENTH }, { ChromaticInterval::Types::MAJOR, ChromaticInterval::Degrees::NINTH } } }, 
		{ { { ChromaticInterval::Types::MINOR, ChromaticInterval::Degrees::THIRD },{ ChromaticInterval::Types::PERFECT, ChromaticInterval::Degrees::FIFTH },{ ChromaticInterval::Types::MINOR, ChromaticInterval::Degrees::SEVENTH },{ ChromaticInterval::Types::MAJOR, ChromaticInterval::Degrees::NINTH } } }, 
		{ { { ChromaticInterval::Types::MAJOR, ChromaticInterval::Degrees::THIRD },{ ChromaticInterval::Types::PERFECT, ChromaticInterval::Degrees::FIFTH },{ ChromaticInterval::Types::MAJOR, ChromaticInterval::Degrees::SEVENTH },{ ChromaticInterval::Types::MAJOR, ChromaticInterval::Degrees::NINTH } } }, 
		//The 6th needs to be +9 semitones
		{ { { ChromaticInterval::Types::MAJOR, ChromaticInterval::Degrees::THIRD },{ ChromaticInterval::Types::MAJOR, ChromaticInterval::Degrees::FIFTH },{ ChromaticInterval::Types::PERFECT, ChromaticInterval::Degrees::SIXTH },{ ChromaticInterval::Types::PERFECT, ChromaticInterval::Degrees::NINTH } } },
		{ { { ChromaticInterval::Types::MINOR, ChromaticInterval::Degrees::THIRD },{ ChromaticInterval::Types::MAJOR, ChromaticInterval::Degrees::FIFTH },{ ChromaticInterval::Types::PERFECT, ChromaticInterval::Degrees::SIXTH },{ ChromaticInterval::Types::PERFECT, ChromaticInterval::Degrees::NINTH } } }
		
		} };

public:
	const Note operator[](const int idx) const {
		switch (idx) {
		case 1:
			return NinthIntervals[ninth][0].get(root);
		case 2:
			return NinthIntervals[ninth][1].get(root);
		case 3:
			return NinthIntervals[ninth][2].get(root);
		case 4:
			return NinthIntervals[ninth][3].get(root);
		default:
			return root;
		}
	}

	void operator=(const ChromaticNinth s_) {
		root = s_.root;
		ninth = s_.ninth;
	}




	const size_t size() const {
		return 4;
	}

#ifdef ATONAL_NAMES
	const std::string name() const {
		return root.name() + " " + NinthTypeEN[static_cast<int>(ninth)];
	}
#endif
};


//Streamlined chord structure, it's missing a bunch of stuff like half diminished, minor/major and other variations
//that don't line up nicely with other chords, these can be dealt with via transposing single notes if required
//Though it includes major and minor sixths and those duplicate major and minor triads
//But I wanted them in there
struct UnifiedChords {
public:
	enum Types {
		MINOR,
		MAJOR,
		DOMINANT,
		AUGMENTED,
		DIMINISHED,
		SUS2,
		SUS4,
		MINORSIXTH,
		MAJORSIXTH,
		DIATONIC,
		UNIFIED_TYPES
	};

private:

	const std::array<std::array<ChromaticInterval, 4>, Types::UNIFIED_TYPES - 1> NinthIntervals = { {
		{ { { ChromaticInterval::Types::MINOR,   ChromaticInterval::Degrees::THIRD },{ ChromaticInterval::Types::PERFECT,    ChromaticInterval::Degrees::FIFTH },{ ChromaticInterval::Types::MINOR,      ChromaticInterval::Degrees::SEVENTH },{ ChromaticInterval::Types::MAJOR, ChromaticInterval::Degrees::NINTH } } },
		{ { { ChromaticInterval::Types::MAJOR,   ChromaticInterval::Degrees::THIRD },{ ChromaticInterval::Types::PERFECT,    ChromaticInterval::Degrees::FIFTH },{ ChromaticInterval::Types::MAJOR,      ChromaticInterval::Degrees::SEVENTH },{ ChromaticInterval::Types::MAJOR, ChromaticInterval::Degrees::NINTH } } },
		{ { { ChromaticInterval::Types::MAJOR,   ChromaticInterval::Degrees::THIRD },{ ChromaticInterval::Types::PERFECT,    ChromaticInterval::Degrees::FIFTH },{ ChromaticInterval::Types::MINOR,      ChromaticInterval::Degrees::SEVENTH },{ ChromaticInterval::Types::MAJOR, ChromaticInterval::Degrees::NINTH } } },
		{ { { ChromaticInterval::Types::MAJOR,   ChromaticInterval::Degrees::THIRD },{ ChromaticInterval::Types::AUGMENTED,  ChromaticInterval::Degrees::FIFTH },{ ChromaticInterval::Types::MINOR,      ChromaticInterval::Degrees::SEVENTH },{ ChromaticInterval::Types::MINOR, ChromaticInterval::Degrees::NINTH } } },
		{ { { ChromaticInterval::Types::MINOR,   ChromaticInterval::Degrees::THIRD },{ ChromaticInterval::Types::DIMINISHED, ChromaticInterval::Degrees::FIFTH },{ ChromaticInterval::Types::DIMINISHED, ChromaticInterval::Degrees::SEVENTH },{ ChromaticInterval::Types::MINOR, ChromaticInterval::Degrees::NINTH } } },
		{ { { ChromaticInterval::Types::MAJOR,   ChromaticInterval::Degrees::SECOND },{ ChromaticInterval::Types::PERFECT,    ChromaticInterval::Degrees::FIFTH },{ ChromaticInterval::Types::MINOR,      ChromaticInterval::Degrees::SEVENTH },{ ChromaticInterval::Types::MAJOR, ChromaticInterval::Degrees::NINTH } } },
		{ { { ChromaticInterval::Types::PERFECT, ChromaticInterval::Degrees::FOURTH },{ ChromaticInterval::Types::PERFECT,    ChromaticInterval::Degrees::FIFTH },{ ChromaticInterval::Types::MINOR,      ChromaticInterval::Degrees::SEVENTH },{ ChromaticInterval::Types::MAJOR, ChromaticInterval::Degrees::NINTH } } },
		{ { { ChromaticInterval::Types::MINOR,   ChromaticInterval::Degrees::THIRD },{ ChromaticInterval::Types::PERFECT,    ChromaticInterval::Degrees::FIFTH },{ ChromaticInterval::Types::MAJOR,      ChromaticInterval::Degrees::SIXTH },{ ChromaticInterval::Types::MAJOR, ChromaticInterval::Degrees::NINTH } } },
		{ { { ChromaticInterval::Types::MAJOR,   ChromaticInterval::Degrees::THIRD },{ ChromaticInterval::Types::PERFECT,    ChromaticInterval::Degrees::FIFTH },{ ChromaticInterval::Types::MAJOR,      ChromaticInterval::Degrees::SIXTH },{ ChromaticInterval::Types::MAJOR, ChromaticInterval::Degrees::NINTH } } }
		} };

public:
	Note root;
	Types type;
	ChromaticScale key; //It's a touch ugly that the key tonic != root note automaticly

	UnifiedChords(const Note root_, const Types type_, const ChromaticScale::ScaleTypes scale_) { root = root_; type = type_; key.tonic = root_; key.scale = scale_; }
	UnifiedChords() { root.n = int(ChromaticNotes::C);  type = Types::MINOR; key.tonic = root; key.scale = ChromaticScale::ScaleTypes::MAJOR; }

	const Note operator[](const int idx) const {

		//
		//Scale getNote is wrapping the octave
		//
		if (type == UnifiedChords::DIATONIC) {
			switch (idx) {
			case 1:
				return key.getNote(key.tonic.n + 2);
			case 2:
				return key.getNote(key.tonic.n + 4);
			case 3:
				return key.getNote(key.tonic.n + 6);
			case 4:
				return key.getNote(key.tonic.n + 8);
			default:
				return key.getNote(key.tonic.n);
			}

		}
		else {
			switch (idx) {
			case 1:
				return NinthIntervals[type][0].get(root);
			case 2:
				return NinthIntervals[type][1].get(root);
			case 3:
				return NinthIntervals[type][2].get(root);
			case 4:
				return NinthIntervals[type][3].get(root);
			default:
				return root;
			}
		}
	}

	void operator=(const UnifiedChords s_) {
		root = s_.root;
		type = s_.type;
	}

	const size_t size() const {
		return 4;
	}

};















