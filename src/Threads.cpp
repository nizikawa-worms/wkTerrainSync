
#include <Windows.h>
#include "Threads.h"
#include "Missions.h"
#include "TerrainList.h"
#include "Water.h"
#include "Config.h"

void Threads::startDataScan() {
	auto task = []{
		auto start = std::chrono::high_resolution_clock::now();
		decltype(start) finish;
		try {
			Missions::createMissionDirs();
			Missions::convertMissionFiles();
			TerrainList::rescanTerrains();
			Water::rescanWaterDirs();
			Config::setModuleInitialized(1);
			finish = std::chrono::high_resolution_clock::now();
		} catch(std::exception & e) {
			finish = std::chrono::high_resolution_clock::now();
			MessageBoxA(0, e.what(), Config::getFullStr().c_str(), MB_ICONERROR);
		}
		std::chrono::duration<double> elapsed = finish - start;
		printf(PROJECT_NAME " data scan took %lf seconds\n", elapsed.count());
	};
	if(Config::isScanTerrainsInBackground()) {
		dataScanThread = std::thread(task);
	} else {
		task();
	}
}

void Threads::awaitDataScan() {
	if(dataScanThread.joinable()) dataScanThread.join();
}
