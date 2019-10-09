// Series GUI.cpp : This file contains the 'main' function. Program execution begins and ends there.
//



#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <minwindef.h>
#include <tchar.h>

#include <iostream>
#include "Console.h"
#include "SeriesGfx.h"

#include "Engine/Series.h"
#include "Input.h"




#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "libsndfile.lib")
#pragma comment(lib, "ogg.lib")
#pragma comment(lib, "vorbis.lib")
#pragma comment(lib, "FLAC.lib")

//static Input input;
static Series sweetTunes;


int WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) 
{

//#if _DEBUG
	startconsole();
//#endif
	
	SeriesGfx serfx(&::sweetTunes);

	sweetTunes.initAudio();
	//sweetTunes.loadConfig("series.json");
	sweetTunes.startSound();
    
	serfx.run();

}
