
#ifndef WKTERRAINSYNC_PACKETS_H
#define WKTERRAINSYNC_PACKETS_H

#include <string>
#include <vector>
#include <set>

typedef unsigned long       DWORD;
class Packets {
private:
	static inline DWORD addrClientSlot;
	static inline DWORD addrHostSlot;
	static inline DWORD addrHostUnicast;
	static inline DWORD addrSendMapData;
	static inline bool internalFlag = false;
	static inline bool magicPacketHandledFlag = false;

	static int __fastcall hookHostLobbyPacketHandler(DWORD This, DWORD EDX, int slot, unsigned char * packet, size_t size);
	static void __fastcall hookHostEndscreenPacketHandler(DWORD This, DWORD EDX, int slot, unsigned char * packet, size_t size);
	static int __fastcall hookClientLobbyPacketHandler(DWORD This, DWORD EDX, unsigned char * packet, size_t size);
	static int __fastcall hookClientEndscreenPacketHandler(DWORD This, DWORD EDX, unsigned char * packet, size_t size);
	static int __fastcall hookInternalSendPacket(DWORD This, DWORD EDX, unsigned char * packet, size_t size);
	static void __stdcall hookSendMapData(int a2);
	static void __stdcall hookSendColorMapData(DWORD hostThis, int slot);

	static inline std::vector<int(__stdcall *)(DWORD, int, unsigned char *, size_t)> hostPacketHandlerCallbacks;
	static inline std::vector<int(__stdcall *)(DWORD, unsigned char *, size_t)> hostInternalPacketHandlerCallbacks;
	static inline std::vector<int(__stdcall *)(DWORD, unsigned char *, size_t)> clientPacketHandlerCallbacks;

	static inline std::set<DWORD> nagStatus;
	static inline DWORD addrHostIncomingConnection;
public:
	static void sendDataToClient_connection(DWORD connection, std::string msg);
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

	static bool fixTerrainId(DWORD This, unsigned char *packet, size_t offset);
	static void sendNagMessage(DWORD connection, const std::string & message);
	static void sendMessage(DWORD connection, const std::string & message);

	static void registerHostPacketHandlerCallback(int(__stdcall * callback)(DWORD HostThis, int slot, unsigned char * packet, size_t size));
	static void registerClientPacketHandlerCallback(int(__stdcall * callback)(DWORD ClientThis, unsigned char * packet, size_t size));
	static void registerHostInteralPacketHandlerCallback(int(__stdcall * callback)(DWORD Connection, unsigned char * packet, size_t size));
	static void clientOnTerrainPacket(DWORD This, DWORD EDX, unsigned char * packet, size_t size);
	static void clearNagStatus();
	static int getNumSlots();
};


#endif //WKTERRAINSYNC_PACKETS_H
