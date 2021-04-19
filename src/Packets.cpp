#include "Packets.h"
#include <utility>
#include "Hooks.h"
#include "Protocol.h"
#include "TerrainList.h"
#include "Frontend.h"
#include "Config.h"
#include "Utils.h"
#include "LobbyChat.h"


int (__fastcall *origHostLobbyPacketHandler)(DWORD This, DWORD EDX, int slot, unsigned char * packet, size_t size);
int __fastcall Packets::hookHostLobbyPacketHandler(DWORD This, DWORD EDX, int slot, unsigned char * packet, size_t size) {
	if(Config::isHexDumpPacketsEnabled()) {
		Utils::hexDump("HostLobbyPacketHandler", packet, size);
	}
	auto ptype = *(unsigned short int*) packet;
	if(ptype == Protocol::magicPacketID) {
		Protocol::parseMsgHost(This, std::string((char*)packet, size), slot);
		return 0;
	} else {
		return origHostLobbyPacketHandler(This, 0, slot, packet, size);
	}
}

void (__fastcall *origHostEndscreenPacketHandler)(DWORD This, DWORD EDX, int slot, unsigned char * packet, size_t size);
void __fastcall Packets::hookHostEndscreenPacketHandler(DWORD This, DWORD EDX, int slot, unsigned char * packet, size_t size) {
	if(Config::isHexDumpPacketsEnabled()) {
		Utils::hexDump("HostEndscreenPacketHandler", packet, size);
	}
	auto ptype = *(unsigned short int*) packet;
	if(ptype == Protocol::magicPacketID) {
		Protocol::parseMsgHost(This, std::string((char*)packet, size), slot);
	} else {
		origHostEndscreenPacketHandler(This, 0, slot, packet, size);
	}
}

int (__fastcall *origClientLobbyPacketHandler)(DWORD This, DWORD EDX, unsigned char * packet, size_t size) ;
int __fastcall Packets::hookClientLobbyPacketHandler(DWORD This, DWORD EDX, unsigned char * packet, size_t size) {
	if(Config::isHexDumpPacketsEnabled()) {
		Utils::hexDump("ClientLobbyPacketHandler", packet, size);
	}
	auto ptype = *(unsigned short int*) packet;
	if(ptype == Protocol::magicPacketID) {
		Protocol::parseMsgClient(std::string((char*)packet, size), This);
		return 0;
	} else {
		return origClientLobbyPacketHandler(This, 0, packet, size);
	}
}

int (__fastcall *origClientEndscreenPacketHandler)(DWORD This, DWORD EDX, unsigned char * packet, size_t size);
int __fastcall Packets::hookClientEndscreenPacketHandler(DWORD This, DWORD EDX, unsigned char * packet, size_t size) {
	if(Config::isHexDumpPacketsEnabled()) {
		Utils::hexDump("ClientEndscreenPacketHandler", packet, size);
	}
	auto ptype = *(unsigned short int*) packet;
	if(ptype == Protocol::magicPacketID) {
		Protocol::parseMsgClient(std::string((char*)packet, size), This);
		return 0;
	} else {
		return origClientEndscreenPacketHandler(This, 0, packet, size);
	}
}

int (__fastcall *origInternalSendPacket)(DWORD This, DWORD EDX, unsigned char * packet, size_t size);
int __fastcall Packets::hookInternalSendPacket(DWORD This, DWORD EDX, unsigned char * packet, size_t size) {
	if(Config::isHexDumpPacketsEnabled()) {
		Utils::hexDump("InternalSendPacket", packet, size);
	}
	if(internalFlag) {
		auto ptype = *(unsigned short int*) packet;
		if(ptype == Protocol::terrainPacketID ) {
			fixTerrainId(This, packet, 0x2C);
			sendNagMessage(This);
		} else if(ptype == Protocol::colormapPacketID) {
			size_t offset = *(size_t*)&packet[8];
			if(offset == 0) {
				int mtype = packet[0x10];
				if(mtype == 1 || mtype == 2) {
					//bit/lev maps
					fixTerrainId(This, packet, 0x34);
					sendNagMessage(This);
				}
			}
		}
	}
	auto ret = origInternalSendPacket(This, 0, packet, size);
	return ret;
}

void Packets::fixTerrainId(DWORD This, unsigned char *packet, size_t offset) {
	int terrainId = packet[offset];
	if(terrainId == 0xFE) {
		packet[offset] = 0xFD;
		Frontend::callMessageBox("wkTerrainSync: The map data is invalid. Please generate a new map.", 0, 0);
		printf("TerrainID is 0xFE --- need to regenerate the map.\n");
	} else if(terrainId >= TerrainList::maxTerrain) {
		packet[offset] = 0xFE;
		sendDataToClient_connection(This, Protocol::dumpTerrainInfo());
	}
}

void Packets::sendDataToClient_connection(DWORD connection, std::string msg) {
	auto data = Protocol::encodeMsg(std::move(msg));
	origInternalSendPacket(connection, 0, (unsigned char*)data.c_str(), data.size());
}

void Packets::sendNagMessage(DWORD connection) {
	if(Config::isNagMessageEnabled()) {
		std::string data = {0x00, 0x00};
		data += LobbyChat::getNagMessage();
		data += {0x00};
		origInternalSendPacket(connection, 0, (unsigned char *) data.c_str(), data.size());
	}
}

int Packets::sendDataToClient_slot(DWORD slot, std::string msg) {
	auto data = Protocol::encodeMsg(std::move(msg));

	size_t sized = data.size();
	auto dataptr = data.c_str();
	int reta;

	_asm push addrHostSlot
	_asm mov esi, dataptr
	_asm mov ecx, slot
	_asm mov edx, sized
	_asm call addrHostUnicast
	_asm mov reta, eax

	return 0;
}

void Packets::sendDataToHost(std::string msg) {
	auto data = Protocol::encodeMsg(std::move(msg));
	(*(int (__thiscall **)(int *, unsigned char *, size_t)) (*(int *) addrClientSlot + 52))((int*)addrClientSlot,(unsigned char*) data.c_str(), data.size());
}


DWORD origSendMapData;
void __stdcall Packets::hookSendMapData(int a2) {
	int a1;
	_asm mov a1, eax

	internalFlag = true;
	_asm mov eax, a1
	_asm push a2
	_asm call origSendMapData
	internalFlag = false;
}

void (__stdcall *origSendColorMapData)(DWORD hostThis, int slot);
void __stdcall Packets::hookSendColorMapData(DWORD hostThis, int slot) {
	internalFlag = true;
	origSendColorMapData(hostThis, slot);
	internalFlag = false;
}

void Packets::resendMapDataToClient(DWORD hostThis, DWORD slot) {
	_asm push slot
	_asm mov eax, hostThis
	_asm call addrSendMapData
}

void Packets::install() {
	DWORD addrHostBroadcast = Hooks::scanPattern("HostBroadcast", "\x83\x78\x0C\x00\x55\x8B\x6C\x24\x0C\x74\x50\x53\x56\xBB\x00\x00\x00\x00\x57\x8D\xB0\x00\x00\x00\x00\x8D\x7B\x05\x8D\x64\x24\x00", "??????xxxxxxxx????xxx????xxxxxxx"); //0x58F8E0
	addrHostUnicast = Hooks::scanPattern("HostUnicast", "\x8B\x44\x24\x04\x83\x78\x0C\x00\x74\x28\x69\xC9\x00\x00\x00\x00\x03\xC1\x83\xB8\x00\x00\x00\x00\x00\x75\x2F\x8D\x88\x00\x00\x00\x00\x8B\x01\x52", "??????xxxxxx????xxxx?????xxxx????xxx"); //0x58F9F0
	DWORD addrHostLobbyPacketHandler = Hooks::scanPattern("HostLobbyPacketHandler", "\x83\xEC\x10\x8B\x44\x24\x1C\x83\xF8\x02\x53\x8B\xD9\x89\x5C\x24\x04\x0F\x82\x00\x00\x00\x00\x55\x8B\x6C\x24\x20\x0F\xB7\x4D\x00\x85\xC9\x56\x57\x0F\x84\x00\x00\x00\x00\x83\xF9\x10\x0F\x84\x00\x00\x00\x00\x83\xF9\x1A\x74\x18", "??????xxxxxxxxxxxxx????xxxxxxxxxxxxxxx????xxxxx????xxxxx"); //0x4B6290
	DWORD addrHostEndscreenPacketHandler = Hooks::scanPattern("HostEndscreenPacketHandler", "\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x83\xEC\x20\x53\x55\x56\x8B\x74\x24\x40\x0F\xB7\x06\x57\x33\xDB\x3B\xC3\x8B\xF9\x89\x7C\x24\x2C\x0F\x84\x00\x00\x00\x00\x83\xF8\x0F\x0F\x85\x00\x00\x00\x00", "???????xx????xxxx????xxxxxxxxxxxxxxxxxxxxxxxxxx????xxxxx????"); //0x4AC0D0
	DWORD addrClientLobbyPacketHandler = Hooks::scanPattern("ClientLobbyPacketHandler", "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x24\x53\x8B\x5D\x0C\x83\xFB\x02\x56\x8B\xF1\x57\x89\x74\x24\x10\x0F\x82\x00\x00\x00\x00\x8B\x7D\x08\x0F\xB7\x0F\x0F\xB7\xC1\x83\xF8\x32\x0F\x87\x00\x00\x00\x00\x0F\xB6\x90\x00\x00\x00\x00\xFF\x24\x95\x00\x00\x00\x00\x53", "??????xxxxxxxxxxxxxxxxxxxx????xxxxxxxxxxxxxx????xxx????xxx????x"); //0x4C0790
	DWORD addrClientEndscreenPacketHandler = Hooks::scanPattern("ClientEndscreenPacketHandler", "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x08\x56\x8B\x75\x08\x0F\xB7\x06\x83\xC0\xE7\x83\xF8\x10\x57\x8B\xF9\x0F\x87\x00\x00\x00\x00\x0F\xB6\x80\x00\x00\x00\x00\xFF\x24\x85\x00\x00\x00\x00\x83\x7D\x0C\x08\x0F\x82\x00\x00\x00\x00\x8B\x4E\x04\x83\xE9\x01\x83\xF9\x0C\x0F\x83\x00\x00\x00\x00", "??????xxxxxxxxxxxxxxxxxxxxx????xxx????xxx????xxxxxx????xxxxxxxxxxx????"); //0x4BD400
	DWORD addrInternalSendPacket = Hooks::scanPattern("InternalSendPacket", "\x53\x8B\x5C\x24\x0C\x55\x56\x57\x8D\x6B\x04\x55\x8B\xF9\xE8\x00\x00\x00\x00\x8B\x54\x24\x18\x8B\xF0\x8D\x43\x04\x66\x89\x46\x02\x53\xC6\x46\x01\x00\x8A\x8F\x00\x00\x00\x00\x52\x8D\x46\x04", "??????xxxxxxxxx????xxxxxxxxxxxxxxxxxxxx????xxxx"); //58FCA0
	DWORD addrSendColorMapData = Hooks::scanPattern("SendColorMapData", "\x55\x8B\x6C\x24\x0C\x69\xED\x00\x00\x00\x00\x57\x8B\xBD\x00\x00\x00\x00\x85\xFF\x0F\x8C\x00\x00\x00\x00\x8B\x44\x24\x0C\x53\x8B\x98\x00\x00\x00\x00\x2B\xDF\x81\xFB\x00\x00\x00\x00\x56\x7E\x05", "??????x????xxx????xxxx????xxxxxxx????xxxx????xxx");
	addrSendMapData = Hooks::scanPattern("SendMapData", "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x2C\x53\x56\x8B\xF0\x8B\x9E\x00\x00\x00\x00\x85\xDB\x57\x7C\x75\x81\xC6\x00\x00\x00\x00\x8D\x7C\x24\x16\x8B\xCB\x8B\xC6\x66\xC7\x44\x24\x00\x00\x00\xE8\x00\x00\x00\x00\x3B\x9E\x00\x00\x00\x00\x7D\x26\x85\xDB\x8B\x86\x00\x00\x00\x00\x8B\xCB\x74\x07", "??????xxxxxxxxx????xxxxxxx????xxxxxxxxxxxx???x????xx????xxxxxx????xxxx");
	addrClientSlot = *(DWORD*)(addrClientEndscreenPacketHandler + 0x103);
	addrHostSlot = *(DWORD*)(addrHostEndscreenPacketHandler + 0xC6);
	printf("addrClientSlot:0x%X addrHostSlot:0x%X\n", addrClientSlot, addrHostSlot);

	Hooks::minhook("HostLobbyPacketHandler", addrHostLobbyPacketHandler, (DWORD*) &hookHostLobbyPacketHandler, (DWORD*)&origHostLobbyPacketHandler);
	Hooks::minhook("HostEndscreenPacketHandler", addrHostEndscreenPacketHandler, (DWORD*) &hookHostEndscreenPacketHandler, (DWORD*)&origHostEndscreenPacketHandler);
	Hooks::minhook("ClientLobbyPacketHandler", addrClientLobbyPacketHandler, (DWORD*) &hookClientLobbyPacketHandler, (DWORD*)&origClientLobbyPacketHandler);
	Hooks::minhook("ClientEndscreenPacketHandler", addrClientEndscreenPacketHandler, (DWORD*) &hookClientEndscreenPacketHandler, (DWORD*)&origClientEndscreenPacketHandler);
	Hooks::minhook("InternalSendPacket", addrInternalSendPacket, (DWORD*) &hookInternalSendPacket, (DWORD*)&origInternalSendPacket);
	Hooks::minhook("SendMapData", addrSendMapData, (DWORD*) &hookSendMapData, (DWORD*)&origSendMapData);
	Hooks::minhook("SendColorMapData", addrSendColorMapData, (DWORD*) &hookSendColorMapData, (DWORD*)&origSendColorMapData);
}

bool Packets::isHost() {
	return *(int *)(addrHostSlot + 12) != 0;
}

bool Packets::isClient() {
	return *(int *)(addrClientSlot + 0xB8) == 3;
}

std::string Packets::getNicknameBySlot(int slot) {
	if(!isHost() || slot < 0 || slot > 6) return "???";
	DWORD slotAddr = addrHostSlot + 0x1A4 + slot * 0x19118;
	return (const char*)(slotAddr + 0xD0);
}
