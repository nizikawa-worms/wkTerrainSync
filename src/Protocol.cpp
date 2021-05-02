
#include "Protocol.h"
#include <json.hpp>
#include "TerrainList.h"
#include "Config.h"
#include "Packets.h"
#include "Frontend.h"
#include "LobbyChat.h"
#include <iostream>
#include <filesystem>
#include <optional>
#include <fstream>
#include <sstream>
#include <Base64.h>

namespace fs = std::filesystem;
typedef unsigned char byte;

void Protocol::install() {

}

std::string Protocol::dumpTerrainInfo() {
	auto & terrainInfo = TerrainList::getLastTerrainInfo();
	nlohmann::json data;
	data["type"] = "TerrainInfo";
	data["hash"] = terrainInfo.hash;
	data["name"] = terrainInfo.name;
	Config::addVersionInfoToJson(data);
	return data.dump();
}

std::string Protocol::encodeMsg(std::string data) {
	ProtocolHeader header{magicPacketID};
	return std::string((const char*)&header, sizeof(header)) + data;
}

std::string Protocol::decodeMsg(std::string data) {
	if(data.size() >= sizeof(ProtocolHeader)) {
		return data.substr(sizeof(ProtocolHeader));
	}
	throw std::runtime_error("Invalid packet size");
}

void Protocol::parseMsgHost(DWORD hostThis, std::string data, int slot) {
	try {
		data = decodeMsg(data);
		nlohmann::json parsed = nlohmann::json::parse(data);
		std::string mType = parsed["type"];
		std::string module = parsed["module"];
		if(module == Config::getModuleStr()) {
			if (mType == "TerrainRequest") {
				handleTerrainRequest(hostThis, parsed, slot);
			} else {
				throw std::runtime_error("Unknown protocol message type: " + mType);
			}
		}
	} catch(std::exception & e) {
		char buff[1024];
		sprintf_s(buff, "wkTerrainSync host exception in data from player %s:\n%s", Packets::getNicknameBySlot(slot).c_str(), e.what());
		printf("parseMsgHost exception: %s\n", e.what());
		printf("Received message: |%s|\n", data.c_str());
		Frontend::callMessageBox(buff, 0, 0);
	}
}

void Protocol::parseMsgClient(std::string data, DWORD connection) {
	try {
		data = decodeMsg(data);
		nlohmann::json parsed = nlohmann::json::parse(data);
		std::string mType = parsed["type"];
		std::string module = parsed["module"];
		if(module == Config::getModuleStr()) {
			if (mType == "TerrainInfo") {
				printf("Received TerrainInfo packet: %s\n", data.c_str());
				std::string thash = parsed["hash"];
				if(thash.empty()) {
					throw std::runtime_error("Received TerrainInfo with empty hash. This is a bug. Data: " + data);
				}
				if (!TerrainList::setLastTerrainInfoByHash(thash)) {
					char buff[512];
					if (Config::isDownloadAllowed()) {

						if(requestedHashes.find(thash) == requestedHashes.end()) {
							requestedHashes.insert(thash);
							printf("Requesting terrain files!\n");
							sprintf_s(buff, "Requesting terrain files from host. Terrain: %s hash: %s", std::string(parsed["name"]).c_str(), std::string(parsed["hash"]).c_str());
							LobbyChat::lobbyPrint(buff);
							nlohmann::json response;
							response["type"] = "TerrainRequest";
							response["terrainHash"] = parsed["hash"];
							Config::addVersionInfoToJson(response);
							Packets::sendDataToHost(response.dump());
						}
					} else {
						printf("Terrain file is missing, but terrain download is disabled in .ini\n");
						sprintf_s(buff, "The terrain file is missing: %s hash: %s installed, but terrain download is disabled in .ini", std::string(parsed["name"]).c_str(), std::string(parsed["hash"]).c_str());
						LobbyChat::lobbyPrint(buff);
					}
				}
			} else if (mType == "TerrainChunk") {
				handleTerrainChunk(parsed);
			} else if (mType == "RescanTerrains") {
//				LobbyChat::lobbyPrint("Rescanning terrain files...");
				TerrainList::rescanTerrains();
			} else {
				throw std::runtime_error("Unknown protocol message type: " + mType);
			}
		}
	} catch(std::exception & e) {
		char buff[1024];
		sprintf_s(buff, "wkTerrainSync client exception:\n%s", e.what());
		printf("parseMsgClient exception: %s\n", e.what());
		printf("Received message: |%s|\n", data.c_str());
		Frontend::callMessageBox(buff, 0, 0);
	}
}


void stripNonAlphaNum(std::string & s) {
	s.erase(std::remove_if(s.begin(), s.end(), []( auto const& c ) -> bool { return !std::isalnum(c); } ), s.end());
}

void Protocol::handleTerrainRequest(DWORD hostThis, nlohmann::json & parsed, int slot) {
	printf("Parsing terrain request\n");
	char buff[512];
	if(!Config::isUploadAllowed()) {
		sprintf_s(buff, "Player %s has sent a terrain file request, but terrain upload is disabled in .ini", Packets::getNicknameBySlot(slot).c_str());
		LobbyChat::lobbyPrint(buff);
		return;
	}
	std::string terrainHash = parsed["terrainHash"];
	if(terrainHash.empty()) {
		throw std::runtime_error("Received TerrainRequest with empty hash. This is a bug.");
	}
	auto & customTerrains = TerrainList::getCustomTerrains();
	auto it = customTerrains.find(terrainHash);
	if(it == customTerrains.end())
		throw std::runtime_error("handleTerrainRequest: Terrain hash not found: " + terrainHash);

	auto & terrainInfo = it->second;
	std::string path = "data\\level\\" + terrainInfo.dirname + "\\";
	nlohmann::json metadata;
	metadata["terrainName"] = terrainInfo.name;
	metadata["terrainHash"] = terrainInfo.hash;
	Config::addVersionInfoToJson(metadata);

	sprintf_s(buff, "Sending terrain files (%s) to player: %s", path.c_str(), Packets::getNicknameBySlot(slot).c_str());
	LobbyChat::lobbyPrint(buff);

	sendFile(slot, path, "text.img", metadata);
	if(terrainInfo.hasWaterDir)
		sendFile(slot, path, "water.dir", metadata);
	sendFile(slot, path, "level.dir", metadata);

	nlohmann::json response;
	response["type"] = "RescanTerrains";
	Config::addVersionInfoToJson(response);
	Packets::sendDataToClient_slot(slot, response.dump());

	Packets::resendMapDataToClient(hostThis, slot);

	printf("Processed terrain request");
}


void Protocol::sendFile(int slot, std::string path, std::string filetype, nlohmann::json metadata) {
	printf("sending file: %s %s\n", path.c_str(), filetype.c_str());
	path += filetype;

	auto fileSize = fs::file_size(path);
	std::ifstream in(path, std::ios::binary);
	if (in.good()) {
		std::stringstream buffer;
		buffer << in.rdbuf();
		in.close();

		metadata["type"] = "TerrainChunk";
		metadata["fileSize"] = fileSize;
		metadata["fileType"] = filetype;

		auto buf = buffer.str();
		for (size_t i = 0; i < buf.size(); i += chunkSizeLimit) {
			auto part = buf.substr(i, chunkSizeLimit);
			metadata["chunkOffset"] = i;
			metadata["chunkData"] = macaron::Base64::Encode(part);
			Packets::sendDataToClient_slot(slot, metadata.dump());
		}
	} else {
		Config::setUploadAllowed(false);
		throw std::runtime_error("Failed to read terrain file - disabling upload functionality. File path: " + path);
	}

}


void Protocol::handleTerrainChunk(nlohmann::json & parsed) {
	if(!Config::isDownloadAllowed())
		throw std::runtime_error("Terrain download is disabled in .ini");
	std::string fileType = parsed["fileType"];

	size_t fSizeLimit = 0;
	const std::string *magic = nullptr;
	std::string fName;

	if (fileType == "level.dir" || fileType == "water.dir") {
		fSizeLimit = maxTerrainDirSize;
		magic = &magicDir;
		fName = fileType;
	} else if (fileType == "text.img") {
		fSizeLimit = maxTextImgSize;
		magic = &magicImg;
		fName = fileType;
	} else
		throw std::runtime_error("handleTerrainChunk: Unknown fileType: " + fileType);

	size_t fileSize = parsed["fileSize"];
	if (fileSize > fSizeLimit)
		throw std::runtime_error("handleTerrainChunk: specified file size: " + std::to_string(fileSize) + " exceeds allowed limit: " + std::to_string(fSizeLimit));

	size_t chunkOffset = parsed["chunkOffset"];
	std::string chunkData;
	macaron::Base64::Decode(parsed["chunkData"], chunkData);
	if (chunkOffset + chunkData.size() > fileSize)
		throw std::runtime_error("handleTerrainChunk: chunkOffset: " + std::to_string(chunkOffset) + " chunkSize: " + std::to_string(chunkData.size()) + " excceds fileSize: " + std::to_string(fileSize));

	if(chunkOffset > 0 && chunkOffset < magic->size())
		throw std::runtime_error("handleTerrainChunk: chunkOffset: " + std::to_string(chunkOffset) + " is within magic header check");

	if(chunkOffset == 0) {
		auto index = chunkData.find(*magic);
		if(index != 0)
			throw std::runtime_error("handleTerrainChunk: magic mismatch!");
	}


	std::string terrainName = parsed["terrainName"];
	stripNonAlphaNum(terrainName);

	std::string terrainHash = parsed["terrainHash"];
	stripNonAlphaNum(terrainHash);

	auto & customTerrains = TerrainList::getCustomTerrains();
	if(customTerrains.find(terrainHash) != customTerrains.end())
		throw std::runtime_error("handleTerrainChunk: Received terrain chunk for already downloaded terrain! Name: " + terrainName + "Hash: " + terrainHash);

	std::string outdir = "data\\level\\" + terrainName + " #" + terrainHash.substr(0, 6) + "\\";
//	printf("handleTerrainChunk: terrainName: |%s| terrainHash: |%s| outdir: |%s|\n", terrainName.c_str(), terrainHash.c_str(), outdir.c_str());

	if(terrainName.empty() || terrainHash.empty())
		throw std::runtime_error("handleTerrainChunk: empty terrain name/hash");


	std::filesystem::create_directories(outdir);
	std::string path = outdir + fName;
	std::string pathwithsuffix = path + ".part";

	FILE * f = fopen(pathwithsuffix.c_str(), "r+b");
	if(!f)
		f = fopen(pathwithsuffix.c_str(), "wb");
	if(!f) {
		Config::setDownloadAllowed(false);
		throw std::runtime_error("Failed to create terrain file - disabling terrain download functionality.\nFile path: " + pathwithsuffix);
	}

	fseek(f, chunkOffset, SEEK_SET);
	fwrite(chunkData.c_str(), 1, chunkData.size(), f);
	if(chunkOffset + chunkData.size() == fileSize) {
		fflush(f);
		fclose(f);

		char buff[512];
		printf("File download complete. Renaming %s to %s\n", pathwithsuffix.c_str(), path.c_str());
		if(rename(pathwithsuffix.c_str(), path.c_str())) {
			throw std::runtime_error("Failed to rename file " + pathwithsuffix + " as " + path);
		}
		sprintf_s(buff, "Downloaded terrain file: %s", path.c_str());
		LobbyChat::lobbyPrint(buff);
	}
	else {
		fclose(f);
	}

//	printf("Written chunk to: %s\n", path.c_str());
}
