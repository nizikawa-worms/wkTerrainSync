
#include "Threads.h"
#include "Missions.h"
#include "TerrainList.h"
#include "Water.h"
#include "Config.h"

void Threads::startDataScan() {
	dataScanThread = std::thread([]{
		Missions::createMissionDirs();
		Missions::convertMissionFiles();
		TerrainList::rescanTerrains();
		Water::rescanWaterDirs();
		Config::setModuleInitialized(1);
	});
}

void Threads::awaitDataScan() {
	if(dataScanThread.joinable()) dataScanThread.join();
}
