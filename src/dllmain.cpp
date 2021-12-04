#include <windows.h>
#include <string>

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
#include "MapGenerator.h"
#include "FrontendDialogs.h"
#include "Scheme.h"
#include "Missions.h"
#include "Debugf.h"
#include "Hooks.h"
#include "Water.h"
#include "Threads.h"
#include <chrono>

void install() {
	auto start_hooks = std::chrono::high_resolution_clock::now();
	srand(time(0) * GetCurrentProcessId());
	debugf("Detected WA installation directory: %s\n", Config::getWaDir().string().c_str());
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
	MapGenerator::install();
	FrontendDialogs::install();
	Scheme::install();
	Missions::install();
	Water::install();
	debugf("Hooks installed - starting data scan thread\n");
	Threads::startDataScan();
}

// Thanks StepS
bool LockGlobalInstance(LPCTSTR MutexName)
{
	if (!CreateMutex(NULL, 0, MutexName)) return 0;
	if (GetLastError() == ERROR_ALREADY_EXISTS) return 0;
	return 1;
}

char LocalMutexName[MAX_PATH];
bool LockCurrentInstance(LPCTSTR MutexName)
{
	_snprintf_s(LocalMutexName, _TRUNCATE,"P%u/%s", GetCurrentProcessId(), MutexName);
	return LockGlobalInstance(LocalMutexName);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	switch(ul_reason_for_call) {
		case DLL_PROCESS_ATTACH: {
			auto start = std::chrono::high_resolution_clock::now();
			decltype(start) finish;
			try {
				Config::readConfig();
				if (Config::isModuleEnabled() && Config::waVersionCheck() && LockCurrentInstance("wkTerrainSync")) {
					Hooks::loadOffsets();
					if (Config::isDevConsoleEnabled()) DevConsole::install();
					install();
					Hooks::saveOffsets();
				}
				finish = std::chrono::high_resolution_clock::now();
			} catch (std::exception &e) {
				finish = std::chrono::high_resolution_clock::now();
				MessageBoxA(0, e.what(), Config::getFullStr().c_str(), MB_ICONERROR);
				Hooks::cleanup();
			}
			std::chrono::duration<double> elapsed = finish - start;
			printf("wkTerrainSync startup took %lf seconds\n", elapsed.count());
		}
		break;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			Hooks::cleanup();
			Threads::awaitDataScan();
		default:
			break;
	}
	return TRUE;
}