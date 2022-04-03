#include <stdexcept>
#include <json.hpp>
#include <optional>
#include "Replay.h"
#include "Hooks.h"
#include "TerrainList.h"
#include "Config.h"
#include "MapGenerator.h"
#include "Missions.h"
#include "Debugf.h"
#include "Threads.h"
#include <Windows.h>

Replay::ReplayOffsets Replay::extractReplayOffsets(char * replayName) {
	ReplayOffsets offsets;

	FILE *f = fopen(replayName, "rb");
	try {
		if (!f)
			throw std::runtime_error("Failed to open replay file: " + std::string(replayName));

		fseek(f, 0, SEEK_END);
		offsets.fileSize = ftell(f);
		fseek(f, 0, SEEK_SET);
		if (offsets.fileSize < 0x10)
			throw std::runtime_error("Replay file is too small");

		fseek(f, offsets.mapChunkOffset, SEEK_SET);
		fread(&offsets.mapChunkSize, sizeof(offsets.mapChunkSize), 1, f);

		offsets.settingsChunkOffset = sizeof(offsets.mapChunkSize) + offsets.mapChunkOffset + offsets.mapChunkSize;
		fseek(f, offsets.settingsChunkOffset, SEEK_SET);
		fread(&offsets.settingsChunkSize, sizeof(offsets.settingsChunkSize), 1, f);

		offsets.terrainChunkOffset = sizeof(size_t) + offsets.settingsChunkOffset + offsets.settingsChunkSize;
		offsets.taskMessageStreamOffset = offsets.settingsChunkOffset + offsets.settingsChunkSize + sizeof(size_t);
		fclose(f);
		return offsets;
	} catch (std::exception &e) {
		debugf("exception: %s\n", e.what());
		if (f)
			fclose(f);
		return offsets;
	}
}

	int (__stdcall *origCreateReplay)(int a1, const char* a2, __time64_t Time);
int __stdcall Replay::hookCreateReplay(int a1, const char* a2, __time64_t Time) {
	lastTime = Time;
	if(flagExitEarly) {
		return 0;
	}

	auto ret = origCreateReplay(a1, a2, Time);

	char * replayName = (char*)(a1 + 0xDF60);

	int hasdata = 0;
	for(auto & cb : hasDataToSaveCallbacks) {
		hasdata |= cb();
	}

	auto & lastTerrainInfo = TerrainList::getLastTerrainInfo();
	if((!lastTerrainInfo || !(lastTerrainInfo && lastTerrainInfo->custom))
		&& !MapGenerator::getScaleXIncrements() && !MapGenerator::getScaleYIncrements()
		&& !Missions::getWamFlag()
		&& !hasdata) {
		debugf("Not saving wkterrainsync metadata in replay file\n");
		return ret;
	}

	auto offsets = extractReplayOffsets(replayName);

	FILE * f = fopen(replayName, "r+b");
	try {
		if(!f)
			throw std::runtime_error("Failed to open replay file: " + std::string(replayName));

		nlohmann::json js;
		Config::addVersionInfoToJson(js);
		TerrainList::onCreateReplay(js);
		MapGenerator::onCreateReplay(js);
		Missions::onCreateReplay(js);

		nlohmann::json js2 = nlohmann::json::array({js});
		std::string data = js2.dump();

		for(auto & cb : createReplayCallbacks) {
			data = cb(data.c_str());
		}

		size_t terrainChunkSize = sizeof(size_t) + sizeof(replayMagic) + data.size();
		size_t newSettingsChunkSize = offsets.settingsChunkSize + terrainChunkSize;
		fseek(f, offsets.settingsChunkOffset, SEEK_SET);
		fwrite(&newSettingsChunkSize, sizeof(newSettingsChunkSize), 1, f);

		fseek(f, offsets.terrainChunkOffset, SEEK_SET);
		fwrite(data.c_str(), data.size(), 1, f);
		fwrite(&terrainChunkSize, sizeof(terrainChunkSize), 1, f);
		fwrite(&replayMagic, sizeof(replayMagic), 1, f);
		debugf("saved wkterrainsync info in replay\n");
		fclose(f);
		return ret;
	} catch(std::exception & e) {
		debugf("exception: %s\n", e.what());
		if(f)
			fclose(f);
		return ret;
	}
}

void Replay::loadInfo(nlohmann::json & json) {
	if(json.contains("module") && json["module"] == Config::getModuleStr()) {
		MapGenerator::onLoadReplay(json);
		Missions::onLoadReplay(json);
		TerrainList::onLoadReplay(json);
	}
}

void Replay::loadReplay(char *replayName) {
	debugf("file: %s\n", replayName);
	auto offsets = extractReplayOffsets(replayName);
	replayPlaybackFlag = true;
	Threads::awaitDataScan();
	if(TerrainList::getTerrainList().empty()) TerrainList::rescanTerrains();
	FILE * f = fopen(replayName, "rb");
	try {
		if (!f)
			throw std::runtime_error("Failed to open replay file: " + std::string(replayName));

		fseek(f, offsets.taskMessageStreamOffset - sizeof(size_t), SEEK_SET);
		unsigned int magic;
		fread(&magic, sizeof(magic), 1, f);
		if(magic == replayMagic) {
			size_t terrainChunkSize;
			fseek(f, offsets.taskMessageStreamOffset - sizeof(size_t)*2, SEEK_SET);
			fread(&terrainChunkSize, sizeof(terrainChunkSize), 1, f);
			if(terrainChunkSize > maxTerrainChunkSize)
				throw std::runtime_error("Terrain chunk size exceeds limit. Size: " + std::to_string(terrainChunkSize));

			fseek(f, offsets.taskMessageStreamOffset - terrainChunkSize, SEEK_SET);

			size_t jsonSize = terrainChunkSize - 2*sizeof(size_t);
			char * data = (char*)malloc(jsonSize + 1);
			data[jsonSize] = 0;
			fread(data, jsonSize, 1, f);

			debugf("Terrain chunk contents: |%s|\n", data);
			nlohmann::json js = nlohmann::json::parse(data);

			if(js.is_array()) {
				for(auto & js2 : js) {
					loadInfo(js2);
				}
			} else {
				loadInfo(js);
			}
			for(auto & cb : loadReplayCallbacks) {
				cb(data);
			}
			free(data);
		}
		fclose(f);
	} catch(std::exception & e) {
		if(f)
			fclose(f);
		debugf("exception: %s\n", e.what());
	}
}

int (__stdcall *origLoadReplay)(int a1, int a2);
int __stdcall Replay::hookLoadReplay(int a1, int a2) {
	if(Config::isReplayMsgBox()) {
		char buff[256];
		_snprintf_s(buff, _TRUNCATE, "Load replay - WA PID: %d", GetCurrentProcessId());
		MessageBoxA(0, buff, buff, 0);
	}
	char * filename = (char*)(a1 + 0xDB60);
	debugf("params: 0x%X , 0x%s\n", a1, filename);
	loadReplay(filename);
	return origLoadReplay(a1, a2);
}

void Replay::install() {
	DWORD addrCreateReplay =  _ScanPattern("CreateReplay", "\x55\x8B\xEC\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x81\xEC\x00\x00\x00\x00\x53\x8B\x5D\x08\x56\x57\x89\x65\xF0\x6A\x00\x68\x00\x00\x00\x00\xC6\x83\x00\x00\x00\x00\x00\xC7\x83\x00\x00\x00\x00\x00\x00\x00\x00", "??????????xx????xxxx????xx????xxxxxxxxxxxx????xx?????xx????????");
	DWORD addrLoadReplay = _ScanPattern("LoadReplay", "\x55\x8D\x6C\x24\x90\x83\xEC\x70\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x81\xEC\x00\x00\x00\x00\x83\x3D\x00\x00\x00\x00\x00\x53\x8B\x5D\x78\x56", "??????xxxxx????xx????xxxx????xx????xx?????xxxxx");
	_HookDefault(CreateReplay);
	_HookDefault(LoadReplay);
}

bool Replay::isReplayPlaybackFlag() {
	return replayPlaybackFlag;
}

void Replay::setFlagExitEarly(bool flagExitEarly) {
	Replay::flagExitEarly = flagExitEarly;
}

__time64_t Replay::getLastTime() {
	return lastTime;
}

void Replay::registerLoadReplayCallback(void(__stdcall * callback)(const char * jsonstr)) {
	loadReplayCallbacks.push_back(callback);
}

void Replay::registerCreateReplayCallback(const char*(__stdcall * callback)(const char * jsonstr)) {
	createReplayCallbacks.push_back(callback);
}

void Replay::registerHasDataToSaveCallback(int(__stdcall * callback)()) {
	hasDataToSaveCallbacks.push_back(callback);
}

