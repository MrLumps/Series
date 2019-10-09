# Series

A hobby sound engine thing. The basic scheme is to make something more like a drum machine that a game performs then a system to efficiently playback audio streams.

You can have N channels and P polyphony. Each channel has an assignable Kit, Measure, and sound engine, as well as an fx chain and an EQ built in. Kits are collections of Notes / samples for the engine to reference. A measure (eventually midi) currently is a step sequencer that triggers gates. Each gate 0-P fires off a given note / sample slot in the sound engine. A bit like how midi deals with drum machines (note 0 is drum 0 etc). The idea is that a 'performance' would have the software coordinate loading of kits and measures to running channels.

Currently relies on:
portaudio
boost pool
libsndfile
nlohmann's json
and
ImGui
GLFW
for gui