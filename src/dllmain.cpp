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
#include "MissionSequences.h"
#include "VFS.h"
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
	MissionSequences::install();
	VFS::install();
	debugf("Hooks installed\n");
}

// Thanks StepS
bool LockGlobalInstance(LPCTSTR MutexName)
{
	if(!Config::isMutexEnabled()) return true;
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
				if (Config::isModuleEnabled() && Config::waVersionCheck() && LockCurrentInstance(PROJECT_NAME)) {
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
			printf(PROJECT_NAME " startup took %lf seconds\n", elapsed.count());
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
