
#ifndef WKTERRAINSYNC_PROTOCOL_H
#define WKTERRAINSYNC_PROTOCOL_H


#include <string>
#include <json.hpp>
#include <set>

typedef unsigned long       DWORD;
class Protocol {
public:
	struct ProtocolHeader {
		unsigned short int pktId;
	};
	static const size_t chunkSizeLimit = 5000;
	static const size_t maxTerrainDirSize = 10 * 1024 * 1024;
	static const size_t maxTextImgSize = 100 * 1024;
	static inline const std::string magicDir = {0x44, 0x49, 0x52, 0x1A};
	static inline const std::string magicImg = {0x49, 0x4D, 0x47, 0x1A};


	static const unsigned short int terrainPacketID = 0x21;
	static const unsigned short int colormapPacketID = 0x2B;
	static const unsigned short int magicPacketID = 0x6666; // Project X
private:
	static inline std::set<std::string> requestedHashes;

public:
	static void handleTerrainRequest(DWORD hostThis, nlohmann::json &, int slot);
	static void handleTerrainChunk(nlohmann::json &);

	static void sendFile(int slot, std::string path, std::string filetype, nlohmann::json metadata);

	static std::string encodeMsg(std::string data);
	static std::string decodeMsg(std::string data);

	static void parseMsgHost(DWORD hostThis, std::string data, int slot);
	static void parseMsgClient(std::string data, DWORD connection);

	static std::string dumpTerrainInfo();

	static void install();
};


#endif //WKTERRAINSYNC_PROTOCOL_H
