#include <stdexcept>
#include <json.hpp>
#include <optional>
#include "Replay.h"
#include "Hooks.h"
#include "TerrainList.h"
#include "Config.h"
#include "MapGenerator.h"

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
		printf("extractReplayOffsets exception: %s\n", e.what());
		if (f)
			fclose(f);
		return offsets;
	}
}

	int (__stdcall *origCreateReplay)(int a1, int a2, __time64_t Time);
int __stdcall Replay::hookCreateReplay(int a1, int a2, __time64_t Time) {
	auto ret = origCreateReplay(a1, a2, Time);

	char * replayName = (char*)(a1 + 0xDF60);
	printf("hookCreateReplay: %s\n", replayName);

	auto & lastTerrainInfo = TerrainList::getLastTerrainInfo();
	if(lastTerrainInfo.hash.empty() && !MapGenerator::getScaleXIncrements() && !MapGenerator::getScaleYIncrements()) {
		printf("Not saving wkterrainsync metadata in replay file\n");
		return ret;
	}

	auto offsets = extractReplayOffsets(replayName);

	FILE * f = fopen(replayName, "r+b");
	try {
		if(!f)
			throw std::runtime_error("Failed to open replay file: " + std::string(replayName));

		nlohmann::json js;
		js["type"] = "TerrainInfo";
		js["name"] = lastTerrainInfo.name;
		js["hash"] = lastTerrainInfo.hash;
		Config::addVersionInfoToJson(js);
		MapGenerator::onCreateReplay(js);
		std::string data = js.dump();

		size_t terrainChunkSize = sizeof(size_t) + sizeof(replayMagic) + data.size();
		size_t newSettingsChunkSize = offsets.settingsChunkSize + terrainChunkSize;
		fseek(f, offsets.settingsChunkOffset, SEEK_SET);
		fwrite(&newSettingsChunkSize, sizeof(newSettingsChunkSize), 1, f);

		fseek(f, offsets.terrainChunkOffset, SEEK_SET);
		fwrite(data.c_str(), data.size(), 1, f);
		fwrite(&terrainChunkSize, sizeof(terrainChunkSize), 1, f);
		fwrite(&replayMagic, sizeof(replayMagic), 1, f);
		printf("hookCreateReplay: saved terrain info in replay\n");
		fclose(f);
		return ret;
	} catch(std::exception & e) {
		printf("hookCreateReplay exception: %s\n", e.what());
		if(f)
			fclose(f);
		return ret;
	}
}

void Replay::loadTerrainInfo(char *replayName) {
	printf("loadTerrainInfo: %s\n", replayName);
	auto offsets = extractReplayOffsets(replayName);
	replayPlaybackFlag = true;
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

			printf("loadTerrainInfo: Terrain chunk contents: |%s|\n", data);
			nlohmann::json terrain = nlohmann::json::parse(data);
			free(data);
			MapGenerator::onLoadReplay(terrain);
			std::string thash = terrain["hash"];
			if(!thash.empty()) {
				if(!TerrainList::setLastTerrainInfoByHash(terrain["hash"])) {
					char buff[2048];
					sprintf_s(buff, "This replay file was recorded using a custom terrain which is currently not installed in your WA directory.\nYou will need to obtain the terrain files in order to play this replay.\nSorry about that.\n\nTerrain metadata: %s", terrain.dump(4).c_str());
					MessageBoxA(0, buff, Config::getFullStr().c_str(), MB_OK | MB_ICONERROR);
				}
			}
		}
		fclose(f);
	} catch(std::exception & e) {
		if(f)
			fclose(f);
		printf("loadTerrainInfo exception: %s\n", e.what());
	}
}


void Replay::install() {
	DWORD addrCreateReplay =  Hooks::scanPattern("CreateReplay", "\x55\x8B\xEC\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x81\xEC\x00\x00\x00\x00\x53\x8B\x5D\x08\x56\x57\x89\x65\xF0\x6A\x00\x68\x00\x00\x00\x00\xC6\x83\x00\x00\x00\x00\x00\xC7\x83\x00\x00\x00\x00\x00\x00\x00\x00", "??????????xx????xxxx????xx????xxxxxxxxxxxx????xx?????xx????????");
	Hooks::hook("CreateReplay", addrCreateReplay, (DWORD *) &hookCreateReplay, (DWORD *) &origCreateReplay);
}

bool Replay::isReplayPlaybackFlag() {
	return replayPlaybackFlag;
}
