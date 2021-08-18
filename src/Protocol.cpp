
#include "Protocol.h"
#include <json.hpp>
#include "TerrainList.h"
#include "Config.h"
#include "Packets.h"
#include "Frontend.h"
#include "LobbyChat.h"
#include "Missions.h"
#include <iostream>
#include <filesystem>
#include <optional>
#include <fstream>
#include <sstream>
#include <Base64.h>
#include "Utils.h"
#include "Debugf.h"

namespace fs = std::filesystem;
typedef unsigned char byte;

void Protocol::install() {

}

std::string Protocol::dumpTerrainInfo() {
	auto & terrainInfo = TerrainList::getLastTerrainInfo();
	if(terrainInfo) {
		nlohmann::json data;
		data["type"] = "TerrainInfo";
		data["hash"] = terrainInfo->hash;
		data["name"] = terrainInfo->name;
		Config::addVersionInfoToJson(data);
		return data.dump();
	} else {
		return "";
	}
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
			if (mType == "TerrainRequest") {handleTerrainRequest(hostThis, parsed, slot);}
			else if(mType == "VersionQuery") { sendVersionInfoSlot(parsed, slot);}
			else if(mType == "VersionInfo") { showVersionInfoSlot(parsed, slot);}
			else throw std::runtime_error("Unknown protocol message type: " + mType + " - if you see this error you probably have an outdated version of wkTerrainSync");
		}
	} catch(std::exception & e) {
		char buff[1024];
		_snprintf_s(buff, _TRUNCATE, "wkTerrainSync host exception in data from player %s:\n%s", Packets::getNicknameBySlot(slot).c_str(), e.what());
		debugf("parseMsgHost exception: %s\n", e.what());
		debugf("Received message: |%s|\n", data.c_str());
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
			if(mType == "TerrainChunk") {handleTerrainChunk(parsed);}
			else if(mType == "TerrainInfo") {handleTerrainInfo(data, parsed);}
			else if(mType == "RescanTerrains") {TerrainList::rescanTerrains();}
			else if(mType == "WamChunk") {handleWamChunk(parsed);}
			else if(mType == "WamReset") {Missions::resetCurrentWam();}
			else if(mType == "WamAttempt") {Missions::setWamAttemptNumber(parsed["WamAttempt"]);}
			else if(mType == "VersionQuery") { sendVersionInfoConnection(parsed, connection);}
			else if(mType == "VersionInfo") { showVersionInfoConnection(parsed, connection);}
			else throw std::runtime_error("Unknown protocol message type: " + mType + " - if you see this error you probably have an outdated version of wkTerrainSync");
		}
	} catch(std::exception & e) {
		char buff[1024];
		_snprintf_s(buff, _TRUNCATE, "wkTerrainSync client exception:\n%s", e.what());
		debugf("parseMsgClient exception: %s\n", e.what());
		debugf("Received message: |%s|\n", data.c_str());
		Frontend::callMessageBox(buff, 0, 0);
	}
}

void Protocol::handleTerrainInfo(const std::string & data, const nlohmann::json &parsed) {
	debugf("Received TerrainInfo packet: %s\n", data.c_str());
	std::string thash = parsed["hash"];
	if(thash.empty()) {
		throw std::runtime_error("Received TerrainInfo with empty hash. This is a bug. Data: " + data);
	}
	if (!TerrainList::setLastTerrainInfoByHash(thash)) {
		char buff[512];
		if (Config::isDownloadAllowed()) {
			if(requestedHashes.find(thash) == requestedHashes.end()) {
				requestedHashes.insert(thash);
				debugf("Requesting terrain files!\n");
				_snprintf_s(buff, _TRUNCATE, "Requesting terrain files from host. Terrain: %s hash: %s", std::string(parsed["name"]).c_str(), std::string(parsed["hash"]).c_str());
				LobbyChat::lobbyPrint(buff);
				nlohmann::json response;
				response["type"] = "TerrainRequest";
				response["terrainHash"] = parsed["hash"];
				Config::addVersionInfoToJson(response);
				Packets::sendDataToHost(response.dump());
			}
		} else {
			debugf("Terrain file is missing, but terrain download is disabled in .ini\n");
			_snprintf_s(buff, _TRUNCATE, "The terrain file is missing: %s hash: %s installed, but terrain download is disabled in .ini", std::string(parsed["name"]).c_str(), std::string(parsed["hash"]).c_str());
			LobbyChat::lobbyPrint(buff);
		}
	}
}


void Protocol::handleTerrainRequest(DWORD hostThis, nlohmann::json & parsed, int slot) {
	debugf("Parsing terrain request\n");
	char buff[512];
	if(!Config::isUploadAllowed()) {
		_snprintf_s(buff, _TRUNCATE, "Player %s has sent a terrain file request, but terrain upload is disabled in .ini", Packets::getNicknameBySlot(slot).c_str());
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
	auto path = terrainInfo->dirpath;

	nlohmann::json metadata;
	metadata["terrainName"] = terrainInfo->name;
	metadata["terrainHash"] = terrainInfo->hash;
	Config::addVersionInfoToJson(metadata);

	_snprintf_s(buff, _TRUNCATE, "Sending terrain files (%s) to player: %s", path.string().c_str(), Packets::getNicknameBySlot(slot).c_str());
	LobbyChat::lobbyPrint(buff);

	sendTerrainFile(slot, path, "text.img", metadata);
	if(terrainInfo->hasWaterDir)
		sendTerrainFile(slot, path, "water.dir", metadata);
	sendTerrainFile(slot, path, "level.dir", metadata);

	nlohmann::json response;
	response["type"] = "RescanTerrains";
	Config::addVersionInfoToJson(response);
	Packets::sendDataToClient_slot(slot, response.dump());

	Packets::resendMapDataToClient(hostThis, slot);

	debugf("Processed terrain request");
}


void Protocol::sendTerrainFile(int slot, std::filesystem::path dirpath, std::string filetype, nlohmann::json metadata) {
	auto path = dirpath / filetype;
	debugf("sending file: %s %s\n", path.string().c_str(), filetype.c_str());

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
		throw std::runtime_error("Failed to read terrain file - disabling upload functionality. File path: " + path.string());
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
	Utils::stripNonAlphaNum(terrainName);

	std::string terrainHash = parsed["terrainHash"];
	Utils::stripNonAlphaNum(terrainHash);

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
		debugf("File download complete. Renaming %s to %s\n", pathwithsuffix.c_str(), path.c_str());
		if(rename(pathwithsuffix.c_str(), path.c_str())) {
			throw std::runtime_error("Failed to rename file " + pathwithsuffix + " as " + path);
		}
		_snprintf_s(buff, _TRUNCATE, "Downloaded terrain file: %s", path.c_str());
		LobbyChat::lobbyPrint(buff);
	}
	else {
		fclose(f);
	}

//	printf("Written chunk to: %s\n", path.c_str());
}


void Protocol::sendWam(DWORD connection) {
	auto & wam = Missions::getWamContents();
	if(wam.empty()) return;

	debugf("Sending WAM to connection: 0x%X\n", connection);
	nlohmann::json json;
	json["type"] = "WamChunk";
	json["fileSize"] = wam.size();;
	Config::addVersionInfoToJson(json);

	for (size_t i = 0; i < wam.size(); i += chunkSizeLimit) {
		if(i == 0) json["WamAttempt"] = Missions::getWamAttemptNumber();
		auto part = wam.substr(i, chunkSizeLimit);
		json["chunkOffset"] = i;
		json["chunkData"] = macaron::Base64::Encode(part);
		Packets::sendDataToClient_connection(connection, json.dump());
	}
	Packets::sendMessage(connection, "SYS::ALL:" + Missions::getWamDescription());
	Packets::sendNagMessage(connection, LobbyChat::getMissionNagMessage());
}


void Protocol::handleWamChunk(nlohmann::json & parsed) {
	size_t fileSize = parsed["fileSize"];
	if (fileSize > maxWamSize)
		throw std::runtime_error("handleWamChunk: specified file size: " + std::to_string(fileSize) + " exceeds allowed limit: " + std::to_string(maxWamSize));

	size_t chunkOffset = parsed["chunkOffset"];
	std::string chunkData;
	macaron::Base64::Decode(parsed["chunkData"], chunkData);
	if (chunkOffset + chunkData.size() > fileSize || wamContents.size() + chunkData.size() > fileSize)
		throw std::runtime_error("handleWamChunk: chunkOffset: " + std::to_string(chunkOffset) + " chunkSize: " + std::to_string(chunkData.size()) + " excceds fileSize: " + std::to_string(fileSize));

	if(chunkOffset == 0) {
		wamContents.clear();
		if(parsed.contains("WamAttempt")) {
			Missions::setWamAttemptNumber(parsed["WamAttempt"]);
		}
	}
	wamContents += chunkData;
	if(chunkOffset + chunkData.size() == fileSize) {
		debugf("WAM download complete.\n");
		static const int magiclen = 8;
		if(wamContents.length() > magiclen) {
			for(int i=0; i < magiclen; i++) {
				unsigned char c = wamContents[i];
				if(!std::isprint(c) && !std::isspace(c)) {
					wamContents.clear();
					throw std::runtime_error("Received a .WAM file with binary characters in header");
				}
			}
		}
		Missions::setWamContents(wamContents);
		wamContents.clear();
		LobbyChat::lobbyPrint((char*)"Received WAM mission data!");
	}
}

void Protocol::sendResetWamToAllPlayers() {
	DWORD hostThis = LobbyChat::getLobbyHostScreen();
	if(!hostThis) {
		debugf("hostThis is null\n");
		return;
	}
	nlohmann::json json;
	json["type"] = "WamReset";
	Config::addVersionInfoToJson(json);
	std::string dump = json.dump();
	for(int i=0; i < Packets::numSlots; i++) {
		Packets::sendDataToClient_slot(i, dump);
	}
}

void Protocol::sendWamAttemptsToAllPlayers(int attempts) {
	DWORD hostThis = LobbyChat::getLobbyHostScreen();
	if(!hostThis) {
		debugf("hostThis is null\n");
		return;
	}
	nlohmann::json json;
	json["type"] = "WamAttempt";
	json["WamAttempt"] = attempts;
	Config::addVersionInfoToJson(json);
	std::string dump = json.dump();
	for(int i=0; i < Packets::numSlots; i++) {
		Packets::sendDataToClient_slot(i, dump);
	}
}

void Protocol::sendVersionInfoConnection(nlohmann::json &parsed, DWORD connection) {
	nlohmann::json json;
	json["type"] = "VersionInfo";
	Config::addVersionInfoToJson(json);
	Packets::sendDataToHost(json.dump());
}

void Protocol::showVersionInfoConnection(nlohmann::json &parsed, DWORD connection) {
	std::stringstream ss;
	ss << "The host is using " << parsed["module"] << " " << parsed["version"] << " (" << parsed["build"] << " )";
	LobbyChat::lobbyPrint((char*)ss.str().c_str());
}

void Protocol::showVersionInfoSlot(nlohmann::json &parsed, int slot) {
	std::stringstream ss;
	std::string nickname = Packets::getNicknameBySlot(slot);
	ss << nickname << " is using " << parsed["module"] << " " << parsed["version"] << " (" << parsed["build"] << " )";
	LobbyChat::lobbyPrint((char*)ss.str().c_str());
}

void Protocol::sendVersionInfoSlot(nlohmann::json &parsed, int slot) {
	nlohmann::json json;
	json["type"] = "VersionInfo";
	Config::addVersionInfoToJson(json);
	Packets::sendDataToClient_slot(slot, json.dump());
}

void Protocol::sendVersionQueryToAllPlayers() {
	DWORD hostThis = LobbyChat::getLobbyHostScreen();
	if(!hostThis) {
		debugf("hostThis is null\n");
		return;
	}
	nlohmann::json json;
	json["type"] = "VersionQuery";
	Config::addVersionInfoToJson(json);
	std::string dump = json.dump();
	for(int i=0; i < Packets::numSlots; i++) {
		Packets::sendDataToClient_slot(i, dump);
	}
}

void Protocol::sendVersionQueryToHost() {
	nlohmann::json json;
	json["type"] = "VersionQuery";
	Config::addVersionInfoToJson(json);
	Packets::sendDataToHost(json.dump());
}
