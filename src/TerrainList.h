
#ifndef WKTERRAINSYNC_TERRAINLIST_H
#define WKTERRAINSYNC_TERRAINLIST_H

#include <string>
#include <map>
#include <vector>
#include <json.hpp>
#include <filesystem>
#include <set>

typedef unsigned long       DWORD;

class TerrainList {
public:
	static inline const std::filesystem::path terrainDir = "data\\level\\";

	struct TerrainInfo {
		bool custom;
		std::string name;
		std::filesystem::path dirpath;
		std::string hash;
		bool hasWaterDir;
		std::string toString() const {
			return "Name: " + name + " Dirpath: |" + dirpath.string() + "| Hash: " + hash + (hasWaterDir ? " (custom water.dir)" : "");
		}
	};
	static const int maxDefaultTerrain = 0x1C;
	inline static const std::string fileExt = ".terrainsync";
private:
	static inline std::shared_ptr<TerrainInfo> lastTerrainInfo;
	static inline bool flagReseedTerrain = false;
	static const inline std::vector<std::string> standardTerrains = {"-Beach", "-Desert", "-Farm", "-Forest", "-Hell", "Art", "Cheese", "Construction", "Desert", "Dungeon", "Easter", "Forest", "Fruit", "Gulf", "Hell", "Hospital", "Jungle", "Manhattan", "Medieval", "Music", "Pirate", "Snow", "Space", "Sports", "Tentacle", "Time", "Tools", "Tribal", "Urban"};
	static inline std::map<std::string, std::shared_ptr<TerrainInfo>> customTerrains;
	static inline std::vector<std::pair<std::string, std::shared_ptr<TerrainInfo>>> terrainList;


	static DWORD __stdcall hookTerrain0(int a1, char* a2, char a3);
	static char* __fastcall hookTerrain2(int id);
	static DWORD __stdcall hookTerrain3(int a1, char *a2, char *a3);
	static DWORD __fastcall hookTerrain4(DWORD * This, int EDX, int a2, DWORD a3);
	static inline DWORD addrTerrain4;

	static void __stdcall hookTerrainRandomSeed(int a2);
//	static int __fastcall hookTerrainRandomSeedIdModifier(int a1, int a2, int a3);
	static void __stdcall hookWriteMapThumbnail(int a1);
	static std::string computeTerrainHash(std::filesystem::path dirname);

	static void __stdcall callEditorLoadTextImg(DWORD** a1, char * a2, DWORD * a3);
	static void __stdcall callEditorAddTerrain(DWORD * a1, DWORD * a2);

	static void patchTerrain4();
	static void __stdcall hookMapTypeChecks();
	static int __stdcall hookQuickCPUTerrain_c();
	static inline DWORD addrWaSeed = 0;
	static char* hookGetMapEditorTerrainPath_c(DWORD a1);
	static char* __stdcall hookGetMapEditorTerrainPath();
public:

	static void rescanTerrains();
	static void install();
	static bool setLastTerrainInfoById(int newTerrain);
	static bool setLastTerrainInfoByHash(std::string md5);
	static void resetLastTerrainInfo();
	static void onFrontendExit();
	static void onLoadReplay(nlohmann::json & config);
	static void onCreateReplay(nlohmann::json &);

	static const std::shared_ptr<TerrainInfo> &getLastTerrainInfo();
	static const std::map<std::string, std::shared_ptr<TerrainInfo>> &getCustomTerrains();
	static const std::vector<std::pair<std::string, std::shared_ptr<TerrainInfo>>> &getTerrainList();
};


#endif //WKTERRAINSYNC_TERRAINLIST_H
