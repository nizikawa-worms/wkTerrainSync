
#ifndef WKTERRAINSYNC_EXPORTS_H
#define WKTERRAINSYNC_EXPORTS_H

#ifdef LIBRARY_EXPORTS
#    define LIBRARY_API  extern "C" __declspec(dllexport)
#else
#    define LIBRARY_API  extern "C" __declspec(dllimport)
#endif
typedef unsigned long       DWORD;


LIBRARY_API const char* __stdcall Module_getModuleVersionInfo();
LIBRARY_API int __stdcall Module_isModuleInitialized();
LIBRARY_API void __stdcall Module_registerModuleInitializedCallback(void(__stdcall * callback)());

// Hooks.cpp
LIBRARY_API DWORD __stdcall Hooks_getOffsetByName(const char * name);
LIBRARY_API DWORD __stdcall Hooks_scanPattern(const char* name, const char* pattern, const char* mask);
LIBRARY_API int __stdcall Hooks_hookFunction(const char * name, DWORD pTarget, DWORD *pDetour, DWORD *ppOriginal);
LIBRARY_API void __stdcall Hooks_hookAsm(DWORD startAddr, DWORD hookAddr);
LIBRARY_API void __stdcall Hooks_hookVtable(const char * classname, int offset, DWORD addr, DWORD hookAddr, DWORD * original);
LIBRARY_API void __stdcall Hooks_patchAsm(DWORD addr, unsigned char *op, size_t opsize);

// Replay.cpp
LIBRARY_API void __stdcall Replay_registerLoadReplayCallback(void(__stdcall * callback)(const char * jsonstr));
LIBRARY_API void __stdcall Replay_registerCreateReplayCallback(const char*(__stdcall * callback)(const char * jsonstr));

//Frontend.cpp
LIBRARY_API void __stdcall Frontend_registerChangeScreenCallback(void(__stdcall * callback)(int screen));
LIBRARY_API int __stdcall Frontend_getCurrentScreen();
LIBRARY_API int __stdcall Frontend_messageBox(const char * message, int a2, int a3); //a2, a3 are probably icon / options like in WINAPI. a2=0, a3=0 is fine.

//LobbyChat.cpp
struct LobbyChatScreens {
	DWORD addrHostScreen;
	DWORD addrClientScreen;
	DWORD addrOfflineScreen;
};

LIBRARY_API LobbyChatScreens __stdcall LobbyChat_getScreenPointers();
LIBRARY_API void __stdcall LobbyChat_lobbyPrint(const char * msg);
LIBRARY_API void __stdcall LobbyChat_hostSendGreentext(const char * msg);
LIBRARY_API void __stdcall LobbyChat_registerClientCommandsCallback(int(__stdcall * callback)(const char * command, const char * args));
LIBRARY_API void __stdcall LobbyChat_registerHostCommandsCallback(int(__stdcall * callback)(const char * command, const char * args));

//Packets.cpp
LIBRARY_API void __stdcall Packets_sendStringToClientConnection(DWORD connection, const char * data);
LIBRARY_API void __stdcall Packets_sendStringToClientSlot(int slot, const char * data);
LIBRARY_API void __stdcall Packets_sendStringToHost(const char * data);
LIBRARY_API int __stdcall Packets_hostBroadcastData(unsigned char * data, size_t size);
LIBRARY_API void __stdcall Packets_registerHostPacketHandler(int(__stdcall * callback)(DWORD HostThis, int slot, unsigned char * packet, size_t size));
LIBRARY_API void __stdcall Packets_registerHostInternalPacketHandler(int(__stdcall * callback)(DWORD connection, unsigned char * packet, size_t size));
LIBRARY_API void __stdcall Packets_registerClientPacketHandler(int(__stdcall * callback)(DWORD ClientThis, unsigned char * packet, size_t size));
LIBRARY_API int __stdcall Packets_isHost();
LIBRARY_API int __stdcall Packets_isClient();

//WaLibc.cpp
LIBRARY_API void* __stdcall WaLibc_malloc(size_t size);
LIBRARY_API void __stdcall WaLibc_free(void * ptr);

//W2App.cpp
struct W2AppObjects {
	DWORD addrDDGame;
	DWORD addrDDDisplay;
	DWORD addrDSSound;
	DWORD addrDDKeyboard;
	DWORD addrDDMouse;
	DWORD addrWavCDRom;
	DWORD addrWSGameNet;
	DWORD addrW2Wrapper;
	DWORD addrGameinfoObject;
	DWORD addrGameGlobal;
};

LIBRARY_API W2AppObjects __stdcall W2App_getGlobalObjectPointers();
LIBRARY_API void __stdcall W2App_registerConstructGameGlobalCallback(void(__stdcall * callback)(DWORD GameGlobal));
LIBRARY_API void __stdcall W2App_registerDestroyGameGlobalCallback(void(__stdcall * callback)());

//Scheme.cpp
struct SchemeInfo {
	int version;
	size_t size;
};
LIBRARY_API SchemeInfo __stdcall Scheme_getSchemeVersionAndSize();
LIBRARY_API DWORD __stdcall Scheme_getSchemeAddr();
LIBRARY_API void __stdcall Scheme_setBuiltinScheme(int id);
LIBRARY_API int __stdcall Scheme_setWscScheme(const char * path, int a2);
LIBRARY_API void __stdcall Scheme_setSchemeName(const char * name);
LIBRARY_API int __stdcall Scheme_sendCurrentSchemeToPlayers();
LIBRARY_API void __stdcall Scheme_refreshLobbyDisplay();

//MapGenerator.cpp
LIBRARY_API void __stdcall MapGenerator_registerOnMapResetCallback(void(__stdcall * callback)(int reason));
LIBRARY_API void __stdcall MapGenerator_registerOnExitMapEditorCallback(void(__stdcall * callback)());

//Sprites.cpp
LIBRARY_API int __stdcall Sprites_checkIfFileExistsInVFS(const char * filename, DWORD vfs);
LIBRARY_API DWORD __stdcall Sprites_loadSpriteFromVFS(DWORD DD_Display, int palette, int index, int a4, DWORD vfs, const char *filename);
LIBRARY_API DWORD __stdcall Sprites_loadSpriteFromTerrain(DWORD DD_Display, int palette, int index, DWORD vfs, const char *filename);
LIBRARY_API void __stdcall Sprites_registerOnLoadSpriteFromVFSCallback(int(__stdcall * callback)(DWORD DD_Display, int palette, int index, int unk, int vfs, const char *filename));
LIBRARY_API void __stdcall Sprites_registerOnLoadSpriteFromTerrainCallback(int(__stdcall * callback)(DWORD DD_Display, int palette, int index, int vfs, const char *filename));



#endif //WKTERRAINSYNC_EXPORTS_H
