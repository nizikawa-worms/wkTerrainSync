
#ifndef WKTERRAINSYNC_PACKETS_H
#define WKTERRAINSYNC_PACKETS_H

#include <string>

typedef unsigned long       DWORD;
class Packets {
private:
	static inline DWORD addrClientSlot;
	static inline DWORD addrHostSlot;
	static inline DWORD addrHostUnicast;
	static inline DWORD addrSendMapData;
	static inline bool internalFlag = false;
	static const int numSlots = 6;

	static int __fastcall hookHostLobbyPacketHandler(DWORD This, DWORD EDX, int slot, unsigned char * packet, size_t size);
	static void __fastcall hookHostEndscreenPacketHandler(DWORD This, DWORD EDX, int slot, unsigned char * packet, size_t size);
	static int __fastcall hookClientLobbyPacketHandler(DWORD This, DWORD EDX, unsigned char * packet, size_t size);
	static int __fastcall hookClientEndscreenPacketHandler(DWORD This, DWORD EDX, unsigned char * packet, size_t size);
	static int __fastcall hookInternalSendPacket(DWORD This, DWORD EDX, unsigned char * packet, size_t size);
	static void __stdcall hookSendMapData(int a2);
	static void __stdcall hookSendColorMapData(DWORD hostThis, int slot);

	static void sendDataToClient_connection(DWORD connection, std::string msg);
	static void sendTerrainNagMessage(DWORD connection);
	static void sendBigMapNagMessage(DWORD connection);

public:
	static int sendDataToClient_slot(DWORD slot, std::string msg);
	static void sendDataToHost(std::string msg);
	static void resendMapDataToClient(DWORD hostThis, DWORD slot);
	static void resendMapDataToAllClients();
	static int callHostBroadcastData(unsigned char * packet, size_t size);
	static void resetPlayerBulbs();

	static void install();

	static bool isHost();
	static bool isClient();
	static std::string getNicknameBySlot(int slot);

	static void fixTerrainId(DWORD This, unsigned char *packet, size_t offset);
};


#endif //WKTERRAINSYNC_PACKETS_H
