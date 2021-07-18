
#include "TerrainList.h"
#include <md5.h>
#include "Hooks.h"
#include <filesystem>
#include <fstream>
#include <json.hpp>
#include <sstream>
#include <optional>
#include "WaLibc.h"
#include "W2App.h"
#include "Replay.h"
#include "Packets.h"
#include "LobbyChat.h"
#include "Config.h"
#include "MapGenerator.h"
#include "Utils.h"
#include "Missions.h"



namespace fs = std::filesystem;
DWORD (__stdcall *origTerrain0)(int a1, char* a2, char a3);
DWORD __stdcall TerrainList::hookTerrain0(int a1, char *a2, char a3) {
	auto ret = origTerrain0(a1, a2, a3);
	auto terrain = *(DWORD *)(a1 + 1320);
//	printf("hookTerrain0: a1:0x%X a2: %s a3:0x%X ret:0x%X   terrainID:0x%X\n", a1, a2, a3, ret, terrain);
	if(terrain != 0xFF && strcmp(a2, "data\\loading.thm") != 0) {
		setLastTerrainInfoById(terrain);
	}

//	char * inputReplayPath = (char*)(W2App::getAddrGameinfoObject() + 0xDB60);
//	if(inputReplayPath && strlen(inputReplayPath)) {
//		Replay::loadTerrainInfo(inputReplayPath);
//	}

	auto v32 = *(DWORD *)(a1 + 1348);
	int maxId = maxTerrain;
	if(Packets::isHost() || (!Packets::isClient()))
		maxId = terrainList.size();

	auto mtype = (DWORD*)(a1 + 1364);
	if(*mtype == 0) {
		return 0;
	}

	if((unsigned int)(v32 + 2) <= 3) {
		int terrainId = *(DWORD *) (a1 + 1320);
		if (Packets::isClient() && (terrainId > maxTerrain && terrainId != 0xFF)) {
			char buff[512];
			sprintf_s(buff, "Error: Map data contains invalid terrain ID (%d). The host might be using legacy wkTerrain38 which is not compatible with wkTerrainSync", terrainId);
			LobbyChat::lobbyPrint(buff);
		}
	}
	return (unsigned int)(v32 + 2) <= 3
		   && (*(DWORD *)(a1 + 1320) <= maxId || (*(DWORD *)(a1 + 1320) == 0xFF && !lastTerrainInfo.hash.empty() ))
		   && *(DWORD *)(a1 + 1352) <= 0x12Cu
		   && (v32 < 0 || !*(DWORD *)(a1 + 1324));
}

char * (__fastcall *origTerrain2)(int a1);
char * __fastcall TerrainList::hookTerrain2(int id) {
	static char buff[MAX_PATH];
	std::string datapath = WaLibc::getWaDataPath();
	if(id > maxTerrain) {
		if(!lastTerrainInfo.dirname.empty()) {
			sprintf_s(buff, "%s%s", terrainDir.c_str(), lastTerrainInfo.dirname.c_str());
//			printf("hookTerrain2: %d (%X) -> %s (lastTerrainInfo)\n", id, id, buff);
			return buff;
		} else {
			printf("hookTerrain2: Magic terrainId specified, but lastTerrainInfo is null\n");
			sprintf_s(buff, "%s\\level\\%s", datapath.c_str(), terrainList[0].c_str()); // load first terrain instead of crashing
			return buff;
		}
	} else {
		if(id < 0 || id > terrainList.size()) {
			printf("hookTerrain2: Invalid terrain id %d (0x%X)\n", id, id);
			sprintf_s(buff, "%s\\level\\%s", datapath.c_str(), terrainList[0].c_str()); //// load first terrain instead of crashing
			return buff;
		}
		sprintf_s(buff, "%s\\level\\%s", datapath.c_str(), terrainList[id].c_str());
//		printf("hookTerrain2: %d -> %s\n", id, buff);
		return buff;
	}
}

DWORD origTerrainRandomSeed;
void TerrainList::hookTerrainRandomSeed(int a2) {
	DWORD a1;
	flagReseedTerrain = true;

	_asm mov a1, esi
	_asm push a2
	_asm call origTerrainRandomSeed
}


void (__stdcall * origWriteMapThumbnail)(int a1);
void __stdcall TerrainList::hookWriteMapThumbnail(int a1) {
	if(flagReseedTerrain) {
		int *terrainId = (int *) (a1 + 1320);
		int oldTerrain = *terrainId;
		int newTerrain = rand() % terrainList.size();

		if((GetAsyncKeyState(VK_CONTROL) & 0x8000) && !customTerrains.empty())
			newTerrain = maxTerrain + 1 +( rand() % (customTerrains.size()));
		else if((GetAsyncKeyState(VK_MENU) & 0x8000))
			newTerrain = rand() % (maxTerrain + 1);

		unsigned int flags = 0;
		if(GetAsyncKeyState(0x44) & 0x8000) {
			flags |= MapGenerator::extraBits::f_DecorHell;
		}
		MapGenerator::setFlagExtraFeatures(flags);
		Missions::onMapReset();
		*terrainId = newTerrain;
		setLastTerrainInfoById(newTerrain);

//		printf("hookWriteMapThumbnail: Replacing terrain id: %d (0x%X) -> %d (0x%X)\n", oldTerrain, oldTerrain, newTerrain, newTerrain);
	}
	origWriteMapThumbnail(a1);
	if(flagReseedTerrain) {
		MapGenerator::onReseedAndWriteMapThumbnail(a1);
	}
	flagReseedTerrain = false;
}

DWORD (__stdcall *origTerrain3)(int a1, char *a2, char *a3);
DWORD __stdcall TerrainList::hookTerrain3(int a1, char *a2, char *a3) {
//	printf("hookTerrain3: a1: 0x%X a2: %s a3: %s\n", a1, a2, a3);
	if(!strcmp(a2, "Custom\\"))
		return origTerrain3(a1, a2, a3);

	size_t count = 0;
	auto v6 = (DWORD**)(a1 + 1684);
	std::string datapath = WaLibc::getWaDataPath();

	for(int i = 0 ; i < terrainList.size(); i++) {
		auto buff = (char*)WaLibc::waMalloc(0x100);
		*(v6 - 256) = (DWORD*)buff;
		snprintf(buff, 0x100, "data\\%s%s\\text.img", a2, terrainList[i].c_str());
		// code for CD edition compatibility
		char buff2[MAX_PATH];
		if(i <= maxTerrain)
			sprintf_s(buff2, "%s\\%s%s\\text.img", datapath.c_str(), a2, terrainList[i].c_str());
		else
			sprintf_s(buff2, 0x100, "data\\%s%s\\text.img", a2, terrainList[i].c_str());
		DWORD v7;
		callEditorLoadTextImg(v6, buff2, &v7);
		if(v7) {
			callEditorAddTerrain((DWORD*)(a1 +192), &v7);
			count++;
		} else {
			WaLibc::waFree(buff);
			printf("Failed to load terrain text.img: %s\n", terrainList[i].c_str());
		}
		v6++;
	}
	return count;
}

DWORD origEditorLoadTextImg;
void __stdcall TerrainList::callEditorLoadTextImg(DWORD **a1, char * a2, DWORD * a3) {
	_asm mov eax, a1
	_asm push a3
	_asm push a2
	_asm call origEditorLoadTextImg
}

DWORD origEditorAddTerrain;
void __stdcall TerrainList::callEditorAddTerrain(DWORD * a1, DWORD * a2) {
	_asm mov esi, a1
	_asm push a2
	_asm call origEditorAddTerrain
}

//DWORD (__fastcall *origTerrain4)(DWORD *This, int EDX, int a2, DWORD a3);
//DWORD TerrainList::hookTerrain4(DWORD *This, int EDX, int a2, DWORD a3) {
//	auto ret = origTerrain4(This, EDX, a2, a3);
//	printf("hookTerrain4: a2:0x%X a3:0x%X ret:0x%X\n", a2, a3, ret);
//	return ret;
//}

bool TerrainList::setLastTerrainInfoById(int newTerrain) {
	lastTerrainInfo.clear();
	if(newTerrain > maxTerrain) {
		if(newTerrain < terrainList.size()) {
			auto dirname = terrainList[newTerrain];
			for(auto & it : customTerrains) {
				auto & terrainInfo = it.second;
				if (terrainInfo.dirname == dirname) {
					lastTerrainInfo = terrainInfo;
					printf("setLastTerrainInfoById: id: %d (0x%X) info: %s\n", newTerrain, newTerrain, lastTerrainInfo.toString().c_str());
					return true;
				}
			}
		}
		printf("setLastTerrainInfoById FAILURE: id: %d (0x%X) info: %s\n", newTerrain, newTerrain, lastTerrainInfo.toString().c_str());
		return false;
	} else {
		printf("setLastTerrainInfoById: id: %d (0x%X) cleared - normal terrain\n", newTerrain, newTerrain);
		return true;
	}
}

bool TerrainList::setLastTerrainInfoByHash(std::string md5) {
	lastTerrainInfo.clear();
	auto it = customTerrains.find(md5);
	if(it != customTerrains.end()) {
		auto & terrainInfo = it->second;
		printf("Found terrain by hash! %s\n", terrainInfo.toString().c_str());
		lastTerrainInfo = terrainInfo;
		return true;
	} else {
		printf("Failed to find terrain by hash: %s\n", md5.c_str());
		return false;
	}
}


void TerrainList::rescanTerrains() {
	customTerrains.clear();
	terrainList = standardTerrains;
	printf("Scanning custom terrains\n");

	for (const auto & entry : fs::recursive_directory_iterator(terrainDir)) {
		if(!entry.is_regular_file()) continue;
		auto filename = entry.path().filename().string();
		std::transform(filename.begin(), filename.end(), filename.begin(), [](unsigned char c){ return std::tolower(c);});
		if(filename == "level.dir") {
			auto dirname = entry.path().parent_path().filename().string();
			if(std::find(terrainList.begin(), terrainList.end(), dirname) == terrainList.end()) {
				auto id = terrainList.size();
				terrainList.push_back(dirname);
				auto hash = computeTerrainHash(dirname);
				std::string name = dirname.substr(0, dirname.find(" #", 0));
				if(name.empty())
					name = dirname;
				bool hasWater = false;
				if(FILE * fw = fopen(std::string(entry.path().parent_path().string() + "/water.dir").c_str(), "rb")) {
					hasWater = true;
					fclose(fw);
				}
				customTerrains[hash] = TerrainInfo{name, dirname, hash, hasWater};
				printf("\t %d (0x%X): %s\n", id, id, customTerrains[hash].toString().c_str());
			}
		}
	}
	patchTerrain4();
}

std::string TerrainList::computeTerrainHash(std::string dirname) {
	Chocobo1::MD5 md5;
	auto level = Utils::readFile(terrainDir + dirname + "/level.dir");
	if(level) {
		md5.addData((*level).c_str(), (*level).size());
	}
	auto water = Utils::readFile(terrainDir + dirname + "/water.dir");
	if(water)
		md5.addData((*water).c_str(), (*water).size());
	md5.finalize();
	return md5.toString();
}

const std::vector<std::string> &TerrainList::getTerrainList() {
	return terrainList;
}

const std::map<std::string, TerrainList::TerrainInfo> &TerrainList::getCustomTerrains() {
	return customTerrains;
}

TerrainList::TerrainInfo &TerrainList::getLastTerrainInfo() {
	return lastTerrainInfo;
}

DWORD addrAddTerrainsOnRebuildWindow;
void TerrainList::patchTerrain4() {
	static std::vector<const char*> terrainPtrList;
	terrainPtrList.clear();
	for(auto & it : terrainList) {
		terrainPtrList.push_back(it.c_str());
	}
	void * ptr = terrainPtrList.data();

	unsigned char size1 = terrainPtrList.size();
	unsigned int size2 = size1;
	Hooks::patchAsm(addrTerrain4 + 0x1B9, &size1, sizeof(size1));
	Hooks::patchAsm(addrTerrain4 +0x207, (unsigned char*)&ptr, sizeof(DWORD));

	Hooks::patchAsm(addrAddTerrainsOnRebuildWindow +0xAA, (unsigned char*)&size2, sizeof(size2));
}

void (__stdcall *origMapTypeChecks)();
void __stdcall TerrainList::hookMapTypeChecks() {
	DWORD sesi;
	_asm mov sesi, esi

	MapGenerator::onMapTypeChecks(sesi);

	DWORD mtype = *(DWORD*)(sesi + 0x554);
	if(Config::isExperimentalMapTypeCheck() && !Replay::isReplayPlaybackFlag()) {
		if(mtype == 3 || (mtype <= 2 && *(DWORD*)(sesi+528) <= maxTerrain)) {
			printf("mtype: %d sesi: %d\n", mtype, *(DWORD*)(sesi+528));
			if(!lastTerrainInfo.hash.empty()) {
				printf("hookMapTypeChecks: clearing lastTerrainInfo\n");
			}
			lastTerrainInfo.clear();
		}
	}
	// 2 random map
	// 1 BIT
	// 3 PNG

	_asm mov esi, sesi
	_asm call origMapTypeChecks
}

void TerrainList::onLoadReplay(nlohmann::json &json) {
	if(json.contains("hash")) {
		std::string thash = json["hash"];
		if(!thash.empty()) {
			if(!TerrainList::setLastTerrainInfoByHash(json["hash"])) {
				char buff[2048];
				sprintf_s(buff, "This replay file was recorded using a custom terrain which is currently not installed in your WA directory.\nYou will need to obtain the terrain files in order to play this replay.\nSorry about that.\n\nTerrain metadata: %s", json.dump(4).c_str());
				MessageBoxA(0, buff, Config::getFullStr().c_str(), MB_OK | MB_ICONERROR);
			}
		}
	}
}

void TerrainList::onCreateReplay(nlohmann::json & json) {
	if(lastTerrainInfo.hash.empty() && lastTerrainInfo.name.empty()) return;
	json["name"] = lastTerrainInfo.name;
	json["hash"] = lastTerrainInfo.hash;
}


void TerrainList::install(){
	DWORD addrTerrainRandomSeed= Hooks::scanPattern("TerrainRandomSeed", "\x83\xEC\x0C\x80\x7C\x24\x00\x00\x53\x55\x57\x74\x79\x6A\x10\xFF\x15\x00\x00\x00\x00\x66\x85\xC0\x7D\x31\x8B\x86\x00\x00\x00\x00\x8B\x8E\x00\x00\x00\x00\x68\x00\x00\x00\x00\x68\x00\x00\x00\x00\x50\x51\xE8\x00\x00\x00\x00", "xxxxxx??xxxxxxxxx????xxxxxxx????xx????x????x????xxx????");
//	DWORD addrTerrainRandomSeedIdModifier = Hooks::scanPattern("TerrainRandomSeedIdModifier", "\x8B\x02\x83\xF8\x01\x7D\x40\x83\xF9\x1A\x75\x1A\xF7\xD8\x1B\xC0\x83\xE0\xFD\x83\xC0\x01\x89\x02\xC1\xE0\x10\x03\xC1\x8B\x4C\x24\x04\x89\x01\xC2\x04\x00", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
	DWORD addrWriteMapThumbnail = Hooks::scanPattern("WriteMapThumbnail", "\x83\xEC\x38\x53\x8B\x5C\x24\x40\x55\x56\x57\x6A\x00\x68\x00\x00\x00\x00\x6A\x02\x6A\x00\x6A\x00\x68\x00\x00\x00\x00\x68\x00\x00\x00\x00\xFF\x15\x00\x00\x00\x00\x8B\xF0\x83\xFE\xFF", "??????xxxxxxxx????xxxxxxx????x????xx????xxxxx");

	DWORD addrTerrain0 =  Hooks::scanPattern("Terrain0", "\x55\x8B\xEC\x83\xE4\xF8\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x51\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x53\x8B\x5D\x08\x56", "??????xxx????xx????xxxx????xx????x????xxxxx");
	DWORD addrTerrain2Reference = Hooks::scanPattern("Terrain2_Ref",  "\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x81\xEC\x00\x00\x00\x00\xA1\x00\x00\x00\x00\x8B\x0D\x00\x00\x00\x00\x8B\x15\x00\x00\x00\x00\x89\x84\x24\x00\x00\x00\x00", "???????xx????xxxx????xx????x????xx????xx????xxx????");
	DWORD addrTerrain2 = addrTerrain2Reference + 0xC10 + *(DWORD*)(addrTerrain2Reference + 0xC0C);
	DWORD addrTerrain3 =  Hooks::scanPattern("Terrain3", "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x0C\x8B\x45\x08\x53\x56\x05\x00\x00\x00\x00\x57\x89\x44\x24\x0C\xC7\x44\x24\x00\x00\x00\x00\x00\x68\x00\x00\x00\x00\xE8\x00\x00\x00\x00", "??????xxxxxxxxx????xxxxxxxx?????x????x????");
	addrTerrain4 = Hooks::scanPattern("Terrain4", "\x55\x8B\xEC\x83\xE4\xF8\x64\xA1\x00\x00\x00\x00\x6A\xFF\x68\x00\x00\x00\x00\x50\x8B\x45\x0C\x64\x89\x25\x00\x00\x00\x00\x83\xEC\x20\x85\xC0\x53\x56\x57\x8B\xD9\x74\x63", "??????xx????xxx????xxxxxxx????xxxxxxxxxxxx");
	addrAddTerrainsOnRebuildWindow = Hooks::scanPattern("AddTerrainsOnRebuildWindow", "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x08\x53\x55\x8B\xD9\x83\xBB\x00\x00\x00\x00\x00\x56\x57\x75\x0B\xA1\x00\x00\x00\x00\x89\x44\x24\x14\xEB\x11\x8B\x8B\x00\x00\x00\x00\x8B\x01\x8B\x40\x18", "??????xxxxxxxxx?????xxxxx????xxxxxxxx????xxxxx");

	DWORD addrMapTypeChecks = Hooks::scanPattern("MapTypeChecks", "\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x81\xEC\x00\x00\x00\x00\xA1\x00\x00\x00\x00\x8B\x0D\x00\x00\x00\x00", "???????xx????xxxx????xx????x????xx????");

	DWORD off1 = *(DWORD*)(addrTerrain3 + 0x103);
	origEditorLoadTextImg = addrTerrain3 + 0x107 + off1;
	origEditorAddTerrain = addrTerrain3 + 0x121 + *(DWORD*)(addrTerrain3 + 0x11D);
	printf("origEditorLoadTextImg: 0x%X origEditorAddTerrain: 0x%X\n", origEditorLoadTextImg, origEditorAddTerrain);

	Hooks::hook("Terrain0", addrTerrain0, (DWORD *) &hookTerrain0, (DWORD *) &origTerrain0);
	Hooks::hook("Terrain2", addrTerrain2, (DWORD *) &hookTerrain2, (DWORD *) &origTerrain2);
	Hooks::hook("Terrain3", addrTerrain3, (DWORD *) &hookTerrain3, (DWORD *) &origTerrain3);
//	Hooks::hook("Terrain4", addrTerrain4, (DWORD*) &hookTerrain4, (DWORD*)&origTerrain4);

	Hooks::hook("TerrainRandomSeed", addrTerrainRandomSeed, (DWORD *) &hookTerrainRandomSeed, (DWORD *) &origTerrainRandomSeed);
	Hooks::hook("WriteMapThumbnail", addrWriteMapThumbnail, (DWORD *) &hookWriteMapThumbnail, (DWORD *) &origWriteMapThumbnail);
//	Hooks::minhook("TerrainRandomSeedIdModifier", addrTerrainRandomSeedIdModifier, (DWORD*) &hookTerrainRandomSeedIdModifier, (DWORD*)&origTerrainRandomSeedIdModifier);
	Hooks::hook("MapTypeChecks", addrMapTypeChecks, (DWORD *) &hookMapTypeChecks, (DWORD *) &origMapTypeChecks);
}

void TerrainList::onFrontendExit() {
	lastTerrainInfo.clear();
}



