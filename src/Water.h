

#ifndef WKTERRAINSYNC_WATER_H
#define WKTERRAINSYNC_WATER_H


#include <string>
#include <filesystem>
#include <vector>

class Water {
private:
	static inline std::vector<std::filesystem::path> customWaterDirs;
	static inline bool scanFinished = false;
public:
	static void install();
	static void rescanWaterDirs();
	static void handleWaFopen(std::string & file, char * mode);
};


#endif //WKTERRAINSYNC_WATER_H
