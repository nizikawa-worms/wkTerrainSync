
#ifndef WKTERRAINSYNC_TERRAINLIST_H
#define WKTERRAINSYNC_TERRAINLIST_H

#include <string>
#include <map>
#include <vector>

typedef unsigned long       DWORD;

class TerrainList {
public:
	static inline const std::string terrainDir = "data\\level\\";

	struct TerrainInfo {
		std::string name;
		std::string dirname;
		std::string hash;
		bool hasWaterDir;
		std::string toString() const {
			return "Name: " + name + " Dirname: |" + dirname + "| Hash: " + hash + (hasWaterDir ? " (custom water.dir)" : "");
		}
		void clear() {
			dirname.clear();
			name.clear();
			hash.clear();
		}
	};
	static const int maxTerrain = 0x1C;
	inline static const std::string fileExt = ".terrainsync";
private:
	static inline TerrainInfo lastTerrainInfo;

	static inline bool flagReseedTerrain = false;

	static const inline std::vector<std::string> standardTerrains = {"-Beach", "-Desert", "-Farm", "-Forest", "-Hell", "Art", "Cheese", "Construction", "Desert", "Dungeon", "Easter", "Forest", "Fruit", "Gulf", "Hell", "Hospital", "Jungle", "Manhattan", "Medieval", "Music", "Pirate", "Snow", "Space", "Sports", "Tentacle", "Time", "Tools", "Tribal", "Urban"};
	static inline std::map<std::string, TerrainInfo> customTerrains;
	static inline std::vector<std::string> terrainList;



	static DWORD __stdcall hookTerrain0(int a1, char* a2, char a3);
	static char* __fastcall hookTerrain2(int id);
	static DWORD __stdcall hookTerrain3(int a1, char *a2, char *a3);
//	static DWORD __fastcall hookTerrain4(DWORD * This, int EDX, int a2, DWORD a3);
	static inline DWORD addrTerrain4;

	static void __stdcall hookTerrainRandomSeed(int a2);
//	static int __fastcall hookTerrainRandomSeedIdModifier(int a1, int a2, int a3);
	static void __stdcall hookWriteMapThumbnail(int a1);
	static std::string computeTerrainHash(std::string dirname);

	static void __stdcall callEditorLoadTextImg(DWORD** a1, char * a2, DWORD * a3);
	static void __stdcall callEditorAddTerrain(DWORD * a1, DWORD * a2);

	static void patchTerrain4();
	static void __stdcall hookMapTypeChecks();
public:

	static void rescanTerrains();
	static void install();

	static const std::vector<std::string> &getTerrainList();
	static const std::map<std::string, TerrainInfo> &getCustomTerrains();
	static TerrainInfo &getLastTerrainInfo();

	static bool setLastTerrainInfoById(int newTerrain);
	static bool setLastTerrainInfoByHash(std::string md5);
};


#endif //WKTERRAINSYNC_TERRAINLIST_H
