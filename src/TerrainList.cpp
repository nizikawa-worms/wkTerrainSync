
#include "TerrainList.h"
#include <md5.h>
#include "Hooks.h"
#include <filesystem>
#include <fstream>
#include <json.hpp>
#include <memory>
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
#include "Debugf.h"
#include "Frontend.h"
#include "Threads.h"
#include "Base64.h"
#include <Windows.h>


namespace fs = std::filesystem;
DWORD (__stdcall *origTerrain0)(int a1, char* a2, char a3);
DWORD __stdcall TerrainList::hookTerrain0(int a1, char *a2, char a3) {
	Threads::awaitDataScan();
	if(terrainList.empty()) rescanTerrains();
	auto ret = origTerrain0(a1, a2, a3);
	auto mtype = (DWORD*)(a1 + 1364);
	DWORD terrainId = *(DWORD *) (a1 + 1320);
//	printf("hookTerrain0: a1:0x%X a2: %s a3:0x%X ret:0x%X   terrainID:0x%X\n", a1, a2, a3, ret, terrain);
	debugf("mtype: 0x%X id: 0x%X\n", *mtype, terrainId);
	if(*mtype == 0) {
		return 0;
	}
	if(*mtype == 1 || *mtype == 2 || (*mtype & 0xFF) == MapGenerator::magicMapType) {
		if (terrainId != 0xFF && strcmp(a2, "data\\loading.thm") != 0 && !Replay::isReplayPlaybackFlag()) {
			setLastTerrainInfoById(terrainId, __CALLPOSITION__);
		}
	} else {
		resetLastTerrainInfo(__CALLPOSITION__);
	}

	auto v32 = *(DWORD *)(a1 + 1348);
	int maxId = maxDefaultTerrain;
	if(Packets::isHost() || (!Packets::isClient()))
		maxId = terrainList.size();


	if((unsigned int)(v32 + 2) <= 3) {
		if (Packets::isClient() && (terrainId > maxDefaultTerrain && terrainId != 0xFF)) {
			char buff[512];
			_snprintf_s(buff, _TRUNCATE, "Error: Map data contains invalid terrain ID (%d). The host might be using legacy wkTerrain38 which is not compatible with " PROJECT_NAME, terrainId);
			LobbyChat::lobbyPrint(buff);
		}
	}
	ret = (unsigned int)(v32 + 2) <= 3
		   && (terrainId <= maxId || (terrainId == 0xFF && lastTerrainInfo) || (Replay::isReplayPlaybackFlag() && lastTerrainInfo))
		   && *(DWORD *)(a1 + 1352) <= 0x12Cu
		   && (v32 < 0 || !*(DWORD *)(a1 + 1324));
	debugf("return: %d\n", ret);
	return ret;
}

char * (__fastcall *origTerrain2)(int a1);
char * __fastcall TerrainList::hookTerrain2(int id) {
	Threads::awaitDataScan();
	if(terrainList.empty()) rescanTerrains();

	static char buff[MAX_PATH];
	std::string datapath = WaLibc::getWaDataPath();
	if(id > maxDefaultTerrain) {
		if(lastTerrainInfo) {
			_snprintf_s(buff, _TRUNCATE, "%s", lastTerrainInfo->dirpath.string().c_str());
//			printf("hookTerrain2: %d (%X) -> %s (lastTerrainInfo)\n", id, id, buff);
			return buff;
		} else {
			debugf("Magic terrainId specified, but lastTerrainInfo is null\n");
			_snprintf_s(buff, _TRUNCATE, "%s\\level\\%s", datapath.c_str(), terrainList[0].first.c_str()); // load first terrain instead of crashing
			return buff;
		}
	} else {
		if(id < 0 || id > terrainList.size()) {
			debugf("Invalid terrain id %d (0x%X)\n", id, id);
			_snprintf_s(buff, _TRUNCATE, "%s\\level\\%s", datapath.c_str(), terrainList[0].first.c_str()); //// load first terrain instead of crashing
			return buff;
		}
		_snprintf_s(buff, _TRUNCATE, "%s\\level\\%s", datapath.c_str(), terrainList[id].first.c_str());
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
	Frontend::refreshMapThumbnailHelpText((CWnd*)a1);
}


void (__stdcall * origWriteMapThumbnail)(int a1);
void __stdcall TerrainList::hookWriteMapThumbnail(int a1) {
	if(flagReseedTerrain) {
		int *terrainId = (int *) (a1 + 1320);
		int oldTerrain = *terrainId;
		int newTerrain = randomTerrainId();
		// force map style
		for(int i=0; i < 8; i++) {
			if((GetAsyncKeyState(0x31 + i) & 0x8000)) {
				*(DWORD *)(a1 + 1316) = (i & 7) >= 4;
				*(DWORD *)(a1 + 1328) = i & 3;
				break;
			}
		}

		unsigned int flags = 0;
		if(GetAsyncKeyState(0x44) & 0x8000) {
			flags |= MapGenerator::extraBits::f_DecorHell;
		}
		MapGenerator::setFlagExtraFeatures(flags);
		Missions::onMapReset();
		*terrainId = newTerrain;
		setLastTerrainInfoById(newTerrain, __CALLPOSITION__);

//		debugf("Replacing terrain id: %d (0x%X) -> %d (0x%X)\n", oldTerrain, oldTerrain, newTerrain, newTerrain);
	}
	origWriteMapThumbnail(a1);
	if(flagReseedTerrain) {
		MapGenerator::onReseedAndWriteMapThumbnail(a1);
	}
	flagReseedTerrain = false;
}

DWORD (__stdcall *origTerrain3)(int a1, char *a2, char *a3);
DWORD __stdcall TerrainList::hookTerrain3(int a1, char *a2, char *a3) {
	Threads::awaitDataScan();
	if(terrainList.empty()) rescanTerrains();
//	printf("hookTerrain3: a1: 0x%X a2: %s a3: %s\n", a1, a2, a3);
	if(!strcmp(a2, "Custom\\"))
		return origTerrain3(a1, a2, a3);

	size_t count = 0;
	auto v6 = (DWORD**)(a1 + 1684);
	std::string datapath = WaLibc::getWaDataPath();

	for(int i = 0 ; i < terrainListWithoutLegacy.size(); i++) {
		auto buff = (char*)WaLibc::waMalloc(0x100);
		char buff2[MAX_PATH];
		*(v6 - 256) = (DWORD*)buff;
		auto & info = terrainList[i].second;

		if(info->custom) {
			snprintf(buff, 0x100, "%s", (info->dirpath / "text.img").string().c_str());
		} else {
			snprintf(buff, 0x100, "%s\\Level\\%s\\text.img", WaLibc::getWaDataPath().c_str(), info->name.c_str());
		}
		strcpy_s(buff2, buff);
		DWORD v7;
		callEditorLoadTextImg(v6, buff2, &v7);
		if(v7) {
			callEditorAddTerrain((DWORD*)(a1 +192), &v7);
			count++;
		} else {
			debugf("Failed to load terrain text.img: %s buff: |%s|\n", terrainListWithoutLegacy[i].first.c_str(), buff);
			WaLibc::waFree(buff);
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

bool TerrainList::setLastTerrainInfoById(int newTerrain, const char * callposition) {
	if(callposition) debugf("Call from: %s\n", callposition);
	Threads::awaitDataScan();
	if(terrainList.empty()) rescanTerrains();
	resetLastTerrainInfo(__CALLPOSITION__);
	if(newTerrain < 0 || newTerrain > terrainList.size()) {
		debugf("invalid terrain id: %d\n", newTerrain);
		return false;
	}
	lastTerrainInfo = terrainList[newTerrain].second;
	debugf("id %d = %s\n", newTerrain, lastTerrainInfo->toString().c_str());
	return true;
}

bool TerrainList::setLastTerrainInfoByHash(std::string md5, const char * callposition) {
	if(callposition) debugf("Call from: %s\n", callposition);
	Threads::awaitDataScan();
	if(terrainList.empty()) rescanTerrains();
	resetLastTerrainInfo(__CALLPOSITION__);
	auto it = std::find_if(terrainList.begin(), terrainList.end(), [&md5](const std::pair<std::string, std::shared_ptr<TerrainInfo>>& element){ return element.second->hash == md5;});
	if(it != terrainList.end()) {
		lastTerrainInfo = it->second;
		debugf("Found terrain by hash! %s\n", lastTerrainInfo->toString().c_str());
		return true;
	}
	else {
		debugf("Failed to find terrain by hash: %s\n", md5.c_str());
		return false;
	}
}

void TerrainList::resetLastTerrainInfo(const char * callposition) {
	if(callposition) debugf("Call from: %s\n", callposition);
	if(lastTerrainInfo) {
		debugf("Resetting lastTerrainInfo (previous value %s)\n", lastTerrainInfo->name.c_str());
	}
	lastTerrainInfo = nullptr;
}



void TerrainList::rescanTerrains() {
	const std::lock_guard<std::mutex> lock(terrainMutex);
	customTerrains.clear();
	terrainList.clear();
	auto wadatapath = WaLibc::getWaDataPath();
	if(wadatapath.empty()) {
		debugf("WA data path not set - rescanTerrains delayed...\n");
		return;
	}
	for(auto & name : standardTerrains) {
		auto path = std::filesystem::path(wadatapath) / "Level" / name;
		bool hasWater = fs::is_regular_file(path / "water.dir");
		bool hasWaterOverride = fs::is_regular_file(path / "_water.dir");
		terrainList.push_back({name, std::make_shared<TerrainInfo>(TerrainInfo{false, name, path, name, hasWater, hasWaterOverride,false})});
	}
	debugf("Scanning custom terrains\n");
	scanTerrainDir(Config::getWaDir() / terrainDir, false);
	scanTerrainDir(Config::getWaDir() / "User/Level", false);
	terrainListWithoutLegacy = terrainList;
	scanTerrainDir(Config::getWaDir() / legacyTerrainDir, true);
	patchTerrain4();
//	int i =0;
//	for(auto & entry : terrainList) {
//		debugf("i: %d entry: %s\n", i, entry.second->toString().c_str());
//		i++;
//	}
}

void TerrainList::scanTerrainDir(std::filesystem::path directory, bool legacy) {
	if(!fs::is_directory(directory)) {
		fs::create_directories(directory);
	}

	for (const auto & entry : fs::recursive_directory_iterator(directory)) {
		if(!entry.is_regular_file()) continue;
		auto filename = entry.path().filename().string();
		std::transform(filename.begin(), filename.end(), filename.begin(), [](unsigned char c){ return tolower(c);});
		if(filename == "level.dir") {
			auto parent = entry.path().parent_path();
			auto dirname = parent.filename().string();
			auto it = std::find_if(terrainList.begin(), terrainList.end(), [&dirname](const std::pair<std::string, std::shared_ptr<TerrainInfo>>& element){ return element.second->name == dirname;} );
			if(it == terrainList.end()) {
				auto id = terrainList.size();
				auto hash = computeTerrainHash(parent);
				std::string name = dirname.substr(0, dirname.find(" #", 0));
				if(name.empty())
					name = dirname;
				bool hasWater = fs::is_regular_file(parent / "water.dir");
				bool hasWaterOverride = fs::is_regular_file(parent / "_water.dir");
				auto info = std::make_shared<TerrainInfo>(TerrainInfo{true, name, parent, hash, hasWater, hasWaterOverride, legacy});
				customTerrains[hash] = info;
				terrainList.push_back({name, info});
				debugf("\t %d (0x%X): %s\n", id, id, info->toString().c_str());
			}
		}
	}
}

std::string TerrainList::computeTerrainHash(std::filesystem::path dirname) {
	Chocobo1::MD5 md5;
	auto level = Utils::readFile(dirname / "level.dir");
	if(level) {
		md5.addData((*level).c_str(), (*level).size());
	}
	auto water = Utils::readFile(dirname / "water.dir");
	if(water)
		md5.addData((*water).c_str(), (*water).size());
	md5.finalize();
	return md5.toString();
}


DWORD addrAddTerrainsOnRebuildWindow;
void TerrainList::patchTerrain4() {
	static std::vector<const char*> terrainPtrList;
	terrainPtrList.clear();
	for(auto & it : terrainListWithoutLegacy) {
		terrainPtrList.push_back(it.first.c_str());
	}
	void * ptr = terrainPtrList.data();

	unsigned char size1 = terrainPtrList.size();
	unsigned int size2 = size1;
	_PatchAsm(addrTerrain4 + 0x1B9, &size1, sizeof(size1));
	_PatchAsm(addrTerrain4 + 0x207, (unsigned char*)&ptr, sizeof(DWORD));
	_PatchAsm(addrAddTerrainsOnRebuildWindow + 0xAA, (unsigned char*)&size2, sizeof(size2));
}

void (__stdcall *origMapTypeChecks)();
void __stdcall TerrainList::hookMapTypeChecks() {
	DWORD sesi;
	_asm mov sesi, esi

	MapGenerator::onMapTypeChecks(sesi);
	DWORD mtype = *(DWORD*)(sesi + 0x554);
	if(Config::isExperimentalMapTypeCheck() && !Replay::isReplayPlaybackFlag()) {
		if(mtype == 3 || (mtype <= 2 && *(DWORD*)(sesi+528) <= maxDefaultTerrain)) {
			if(lastTerrainInfo) {
				debugf("clearing lastTerrainInfo\n");
				resetLastTerrainInfo(__CALLPOSITION__);
			}
		}
	}

	_asm mov esi, sesi
	_asm call origMapTypeChecks
}

void TerrainList::onLoadReplay(nlohmann::json &json) {
	std::string thash = json.contains("hash") ? json["hash"] : "";
	std::string name = json.contains("name") ? json["name"] : "???";

	try {
		if(!thash.empty()) {
			if(!TerrainList::setLastTerrainInfoByHash(json["hash"], __CALLPOSITION__)) {
				if(json.contains("TerrainFiles")) {
					auto & assets = json["TerrainFiles"];
					if(assets.contains("Level.dir") && assets.contains("Text.img")) {
						std::string terrainName = name;
						Utils::stripNonAlphaNum(terrainName);
						std::string terrainHash = thash;
						Utils::stripNonAlphaNum(terrainHash);
						std::filesystem::path outdir = Config::getWaDir() / std::format("{}/{} #{}/", (Config::isExtractTerrainFromReplaysToTmpDir() ? "Data/Level_Replay" : "Data/Level"), terrainName, terrainHash.substr(0, 6));
						debugf("Installing terrain from replay: %s - %s\n", terrainName.c_str(), outdir.string().c_str());
						std::filesystem::create_directories(outdir);
						Utils::decompress_file(assets["Level.dir"], outdir/"Level.dir");
						Utils::decompress_file(assets["Text.img"], outdir/"Text.img");
						if(assets.contains("Water.dir")) Utils::decompress_file(assets["Water.dir"], outdir/"Water.dir");
//						rescanTerrains();
						auto info = std::make_shared<TerrainInfo>(TerrainInfo{true, name, outdir, thash, assets.contains("Water.dir"), false, false});
						customTerrains[thash] = info;
						terrainList.push_back({name, info});
						terrainListWithoutLegacy.push_back({name, info});

						if(!TerrainList::setLastTerrainInfoByHash(json["hash"], __CALLPOSITION__)) throw std::runtime_error("Failed to install terrain files from replay data");
					} else {
						throw std::runtime_error("Replay contains corrupted terrain file data");
					}
				}
				else {
					throw std::runtime_error("This replay file was recorded using a custom terrain which is currently not installed in your WA directory.\nYou will need to obtain the terrain files in order to play this replay.");
				}
			}
		}
	} catch(std::exception & e) {
		std::string meta = std::format("Failed to load custom terrain data.\nTerrain name: {}\nTerrain hash: {}\n\n{}", name, thash, e.what());
		MessageBoxA(0, meta.c_str(), Config::getFullStr().c_str(), MB_OK | MB_ICONERROR);
	}


}

void TerrainList::onCreateReplay(nlohmann::json & json) {
	if(!lastTerrainInfo) return;
	json["name"] = lastTerrainInfo->name;
	json["hash"] = lastTerrainInfo->hash;

	if(Config::isStoreTerrainFilesInReplay()) {
		try {
			auto assets = nlohmann::json();
			assets["Level.dir"] = macaron::Base64::Encode(Utils::compress_file(lastTerrainInfo->dirpath / "Level.dir"));
			assets["Text.img"] = macaron::Base64::Encode(Utils::compress_file(lastTerrainInfo->dirpath / "Text.img"));
			if(lastTerrainInfo->hasWaterDir) {
				assets["Water.dir"] = macaron::Base64::Encode(Utils::compress_file(lastTerrainInfo->dirpath / "Water.dir"));
			}
			json["TerrainFiles"] = assets;
		} catch(std::exception & e) {
			debugf("Failed to compress terrain assets: %s\n", e.what());
		}
	}

}

DWORD addrQuickCPUTerrain_ret;
int __stdcall TerrainList::hookQuickCPUTerrain_c() {
	Threads::awaitDataScan();
	if(terrainList.empty()) rescanTerrains();
	auto seed = (DWORD*)addrWaSeed;
	DWORD v4 = *seed;
	WaLibc::waSrand(*seed);
	DWORD v5 = WaLibc::waRand() << 16;
	*seed = v5 + WaLibc::waRand();
	DWORD terrain = v4 % 0x1D;

	if(!Replay::isReplayPlaybackFlag()) {
		int newTerrain = randomTerrainId();
		if(newTerrain >= 0x1D && Config::isUseCustomTerrainsInSinglePlayerMode()) {
			setLastTerrainInfoById(newTerrain, __CALLPOSITION__);
			return newTerrain;
		} else {
			resetLastTerrainInfo(__CALLPOSITION__);
			return terrain;
		}
	} else {
		if(!lastTerrainInfo || !lastTerrainInfo->custom) {return terrain;}
		else {return 0xFF;}
	}
}

int TerrainList::randomTerrainId() {
	auto hasCustom = (terrainListWithoutLegacy.size() - 1 - maxDefaultTerrain) > 0;
	int minId = 0;
	int maxId = terrainListWithoutLegacy.size() - 1;
	if((GetAsyncKeyState(VK_CONTROL) & 0x8000) && hasCustom) {
		minId = maxDefaultTerrain + 1;
		maxId = terrainListWithoutLegacy.size() - 1;
	}
	else if((GetAsyncKeyState(VK_MENU) & 0x8000)) {
		minId = 0;
		maxId = maxDefaultTerrain;
	}
//	debugf("minId: %d maxId: %d\n", minId, maxId);
	return (rand() % (maxId + 1 - minId)) + minId;
}

void __declspec(naked) hookQuickCPUTerrain() {
	_asm call TerrainList::hookQuickCPUTerrain_c
	_asm mov esi, eax
	_asm jmp addrQuickCPUTerrain_ret
}


char* (__stdcall *origGetMapEditorTerrainPath)();
char* TerrainList::hookGetMapEditorTerrainPath_c(DWORD a1) {
	static char buff[MAX_PATH];
	int id = (*(int (__thiscall **)(DWORD))(*(DWORD *)(a1 + 84) + 44))(a1 + 84);
	auto info = terrainList[id].second;
	_snprintf_s(buff, _TRUNCATE, "%s", info->dirpath.string().c_str());
	return buff;
}
#pragma optimize( "", off )
char* __stdcall TerrainList::hookGetMapEditorTerrainPath() {
	DWORD a1;
	_asm mov a1, eax
	return hookGetMapEditorTerrainPath_c(a1);
}
#pragma optimize( "", on )

char *(__fastcall *origSetBuiltinMap)(void *mapname, DWORD a2);
char *__fastcall hookSetBuiltinMap(void *mapname, DWORD a2) {
	TerrainList::resetLastTerrainInfo(__CALLPOSITION__);
	return origSetBuiltinMap(mapname, a2);
}

void TerrainList::install(){
	DWORD addrTerrainRandomSeed= _ScanPattern("TerrainRandomSeed", "\x83\xEC\x0C\x80\x7C\x24\x00\x00\x53\x55\x57\x74\x79\x6A\x10\xFF\x15\x00\x00\x00\x00\x66\x85\xC0\x7D\x31\x8B\x86\x00\x00\x00\x00\x8B\x8E\x00\x00\x00\x00\x68\x00\x00\x00\x00\x68\x00\x00\x00\x00\x50\x51\xE8\x00\x00\x00\x00", "xxxxxx??xxxxxxxxx????xxxxxxx????xx????x????x????xxx????");
//	DWORD addrTerrainRandomSeedIdModifier = ScanPatternf("TerrainRandomSeedIdModifier", "\x8B\x02\x83\xF8\x01\x7D\x40\x83\xF9\x1A\x75\x1A\xF7\xD8\x1B\xC0\x83\xE0\xFD\x83\xC0\x01\x89\x02\xC1\xE0\x10\x03\xC1\x8B\x4C\x24\x04\x89\x01\xC2\x04\x00", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
	DWORD addrWriteMapThumbnail = _ScanPattern("WriteMapThumbnail", "\x83\xEC\x38\x53\x8B\x5C\x24\x40\x55\x56\x57\x6A\x00\x68\x00\x00\x00\x00\x6A\x02\x6A\x00\x6A\x00\x68\x00\x00\x00\x00\x68\x00\x00\x00\x00\xFF\x15\x00\x00\x00\x00\x8B\xF0\x83\xFE\xFF", "??????xxxxxxxx????xxxxxxx????x????xx????xxxxx");

	DWORD addrTerrain0 =  _ScanPattern("Terrain0", "\x55\x8B\xEC\x83\xE4\xF8\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x51\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x53\x8B\x5D\x08\x56", "??????xxx????xx????xxxx????xx????x????xxxxx");
	DWORD addrTerrain2Reference = _ScanPattern("Terrain2_Ref", "\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x81\xEC\x00\x00\x00\x00\xA1\x00\x00\x00\x00\x8B\x0D\x00\x00\x00\x00\x8B\x15\x00\x00\x00\x00\x89\x84\x24\x00\x00\x00\x00", "???????xx????xxxx????xx????x????xx????xx????xxx????");
	DWORD addrTerrain2 = addrTerrain2Reference + 0xC10 + *(DWORD*)(addrTerrain2Reference + 0xC0C);
	DWORD addrTerrain3 =  _ScanPattern("Terrain3", "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x0C\x8B\x45\x08\x53\x56\x05\x00\x00\x00\x00\x57\x89\x44\x24\x0C\xC7\x44\x24\x00\x00\x00\x00\x00\x68\x00\x00\x00\x00\xE8\x00\x00\x00\x00", "??????xxxxxxxxx????xxxxxxxx?????x????x????");
	addrTerrain4 = _ScanPattern("Terrain4", "\x55\x8B\xEC\x83\xE4\xF8\x64\xA1\x00\x00\x00\x00\x6A\xFF\x68\x00\x00\x00\x00\x50\x8B\x45\x0C\x64\x89\x25\x00\x00\x00\x00\x83\xEC\x20\x85\xC0\x53\x56\x57\x8B\xD9\x74\x63", "??????xx????xxx????xxxxxxx????xxxxxxxxxxxx");
	addrAddTerrainsOnRebuildWindow = _ScanPattern("AddTerrainsOnRebuildWindow", "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x08\x53\x55\x8B\xD9\x83\xBB\x00\x00\x00\x00\x00\x56\x57\x75\x0B\xA1\x00\x00\x00\x00\x89\x44\x24\x14\xEB\x11\x8B\x8B\x00\x00\x00\x00\x8B\x01\x8B\x40\x18", "??????xxxxxxxxx?????xxxxx????xxxxxxxx????xxxxx");
	DWORD addrMapTypeChecks = _ScanPattern("MapTypeChecks", "\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x81\xEC\x00\x00\x00\x00\xA1\x00\x00\x00\x00\x8B\x0D\x00\x00\x00\x00", "???????xx????xxxx????xx????x????xx????");
	DWORD addrQuickCPUTerrain = _ScanPattern("QuickCPUTerrain", "\xA1\x00\x00\x00\x00\x83\xF8\x2B\x56\x57\x7D\x5C\x83\xF8\xFC\x74\x57\x8B\x35\x00\x00\x00\x00\x56\xE8\x00\x00\x00\x00\x83\xC4\x04", "??????xxxxxxxxxxxxx????xx????xxx");
	DWORD addrGetMapEditorTerrainPath = _ScanPattern("GetMapEditorTerrainPath", "\x56\x8B\xF0\x8B\x46\x54\x8B\x50\x2C\x8D\x4E\x54\x57\x33\xFF\xFF\xD2\x8B\x96\x00\x00\x00\x00\x85\xD2\x75\x04\x33\xC9\xEB\x0B\x8B\x8E\x00\x00\x00\x00\x2B\xCA\xC1\xF9\x02", "??????xxxxxxxxxxxxx????xxxxxxxxxx????xxxxx");
	DWORD addrSetBuiltinMap = _ScanPattern("SetBuiltinMap", "\x55\x8B\xEC\x83\xE4\xF8\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x51\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x53\x8B\x5D\x08\x56\x8B\xB3\x00\x00\x00\x00\x85\xF6\x57\x8B\xF9\x74\x27\x8B\x06\x83\xE8\x10\x8D\x48\x0C\x83\xCA\xFF", "??????xxx????xx????xxxx????xx????x????xxxxxxx????xxxxxxxxxxxxxxxxxx");
	DWORD off1 = *(DWORD*)(addrTerrain3 + 0x103);
	origEditorLoadTextImg = addrTerrain3 + 0x107 + off1;
	origEditorAddTerrain = addrTerrain3 + 0x121 + *(DWORD*)(addrTerrain3 + 0x11D);
	debugf("origEditorLoadTextImg: 0x%X origEditorAddTerrain: 0x%X\n", origEditorLoadTextImg, origEditorAddTerrain);

	_HookDefault(Terrain0);
	_HookDefault(Terrain2);
	_HookDefault(Terrain3);
//	Hook("Terrain4", addrTerrain4, (DWORD*) &hookTerrain4, (DWORD*)&origTerrain4);
	_HookDefault(TerrainRandomSeed);
	_HookDefault(WriteMapThumbnail);
//	Hook("TerrainRandomSeedIdModifier", addrTerrainRandomSeedIdModifier, (DWORD*) &hookTerrainRandomSeedIdModifier, (DWORD*)&origTerrainRandomSeedIdModifier);
	_HookDefault(MapTypeChecks);
	_HookDefault(GetMapEditorTerrainPath);
	_HookDefault(SetBuiltinMap);

	addrWaSeed = *(DWORD*)(addrQuickCPUTerrain + 0x13);
	DWORD addrQuickCPUTerrains_patch1 = addrQuickCPUTerrain + 0X68;
	addrQuickCPUTerrain_ret = addrQuickCPUTerrain + 0x9C;
	debugf("addrQuickCPUTerrains_patch1: 0x%X ret: 0x%X waseed: 0x%X\n", addrQuickCPUTerrains_patch1, hookQuickCPUTerrain, addrWaSeed);
	_HookAsm(addrQuickCPUTerrains_patch1, (DWORD) &hookQuickCPUTerrain);
}

void TerrainList::onFrontendExit() {
	resetLastTerrainInfo(__CALLPOSITION__);
}

const std::shared_ptr<TerrainList::TerrainInfo> &TerrainList::getLastTerrainInfo() {
	return lastTerrainInfo;
}

const std::map<std::string, std::shared_ptr<TerrainList::TerrainInfo>> &TerrainList::getCustomTerrains() {
	return customTerrains;
}

const std::vector<std::pair<std::string, std::shared_ptr<TerrainList::TerrainInfo>>> &TerrainList::getTerrainList() {
	return terrainList;
}
