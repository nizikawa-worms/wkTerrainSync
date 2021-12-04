
#include "Water.h"
#include "TerrainList.h"
#include "Config.h"
#include "Debugf.h"
#include "WaLibc.h"
#include "Threads.h"

namespace fs = std::filesystem;

void Water::install() {
}

void Water::handleWaFopen(std::string &file, char *mode) {
	if(file == "data\\Gfx\\Water.dir" && Config::isCustomWaterAllowed()) {
		Threads::awaitDataScan();
		auto terraininfo = TerrainList::getLastTerrainInfo();
		if(terraininfo && terraininfo->hasWaterDirOverride && fs::is_regular_file(terraininfo->dirpath / "_water.dir")) {
			file = (terraininfo->dirpath/ "_water.dir").string();
			debugf("using %s (terrain overridden _water.dir)\n", file.c_str());
		}
		else if(terraininfo && terraininfo->hasWaterDir && fs::is_regular_file(terraininfo->dirpath / "water.dir")) {
			file = (terraininfo->dirpath/ "water.dir").string();
			debugf("using %s (terrain water.dir)\n", file.c_str());
		} else if(!customWaterDirs.empty()) {
			size_t index = rand() % customWaterDirs.size();
			if(fs::is_regular_file(customWaterDirs[index])) {
				file = customWaterDirs[index].string();
				debugf("using %s (random custom water.dir)\n", file.c_str());
			}
		}
	}
}

void Water::rescanWaterDirs() {
	customWaterDirs.clear();
	auto waterDir = Config::getWaDir() / "User\\Water";
	fs::create_directories(waterDir);
	debugf("Scanning custom water: %s\n", waterDir.string().c_str());
	for (const auto &entry: fs::recursive_directory_iterator(waterDir)) {
		if(!entry.is_regular_file()) continue;
		auto extension = entry.path().extension().string();
		std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return std::tolower(c); });
		if (extension == ".dir") {
			customWaterDirs.push_back(entry);
			debugf("\tCustom water: %s\n", entry.path().string().c_str());
		}
	}
}