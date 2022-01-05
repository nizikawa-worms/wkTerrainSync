
#include "Threads.h"
#include "Missions.h"
#include "TerrainList.h"
#include "Water.h"
#include "Config.h"

void Threads::startDataScan() {
	auto task = []{
		Missions::createMissionDirs();
		Missions::convertMissionFiles();
		TerrainList::rescanTerrains();
		Water::rescanWaterDirs();
		Config::setModuleInitialized(1);
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
