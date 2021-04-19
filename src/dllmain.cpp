#include <Windows.h>
#include <string>
#include <MinHook.h>
#include <stdexcept>
#include <ctime>

#include "WaLibc.h"
#include "DevConsole.h"
#include "Config.h"
#include "Packets.h"
#include "TerrainList.h"
#include "LobbyChat.h"
#include "Protocol.h"
#include "Replay.h"
#include "Frontend.h"
#include "BitmapImage.h"
#include "W2App.h"
#include "Sprites.h"

void install() {
	if (MH_Initialize() != MH_OK)
		throw std::runtime_error("Failed to initialize Minhook");
	srand(time(0) * GetCurrentProcessId());
	WaLibc::install();
	TerrainList::install();
	Packets::install();
	LobbyChat::install();
	Protocol::install();
	Replay::install();
	Frontend::install();
	BitmapImage::install();
	W2App::install();
	Sprites::install();
	TerrainList::rescanTerrains();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	switch(ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
			try {
				Config::readConfig();
				if(Config::isModuleEnabled() && Config::waVersionCheck()) {
					if(Config::isDevConsoleEnabled()) DevConsole::install();
					install();
				}
			} catch(std::exception & e) {
				MessageBoxA(0, e.what(), Config::getFullStr().c_str(), MB_ICONERROR);
			}
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
		default:
			break;
	}
	return TRUE;
}
