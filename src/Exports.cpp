#include "Exports.h"
#include "Config.h"
#include "Hooks.h"
#include "Replay.h"
#include "Frontend.h"
#include "LobbyChat.h"
#include "Packets.h"
#include "WaLibc.h"
#include "W2App.h"
#include "Scheme.h"
#include "MapGenerator.h"
#include "Sprites.h"

extern "C" {

	const char* __stdcall Module_getModuleVersionInfo() {
		nlohmann::json json;
		Config::addVersionInfoToJson(json);
		static std::string dump = json.dump();
		return dump.c_str();
	}

	int __stdcall Module_isModuleInitialized() {
		return Config::getModuleInitialized();
	}

	void __stdcall Module_registerModuleInitializedCallback(void(__stdcall * callback)()) {
		Config::registerModuleInitializedCallback(callback);
	}

	// Hooks.cpp
	DWORD __stdcall Hooks_getOffsetByName(const char *name) {
		auto &nameToAddr = Hooks::getScanNameToAddr();
		std::string namestr = name;
		if(nameToAddr.find(name) != nameToAddr.end()) {
			return nameToAddr.at(namestr);
		}
		return 0;
	}

	DWORD __stdcall Hooks_scanPattern(const char *name, const char *pattern, const char *mask) {
		try {
			return Hooks::scanPattern(name, pattern, mask);
		} catch(std::exception & e) {
			return 0;
		}
	}

	int __stdcall Hooks_hookFunction(const char * name, DWORD pTarget, DWORD *pDetour, DWORD *ppOriginal) {
		try {
			_Hook(name, pTarget, pDetour, ppOriginal);
			return 1;
		} catch(std::exception & e) {
			return 0;
		}
	}

	void __stdcall Hooks_hookAsm(DWORD startAddr, DWORD hookAddr) {
		_HookAsm(startAddr, hookAddr);
	}

	void __stdcall Hooks_hookVtable(const char * classname, int offset, DWORD addr, DWORD hookAddr, DWORD * original) {
		Hooks::hookVtable(classname, offset, addr, hookAddr, original);
	}

	void __stdcall Hooks_patchAsm(DWORD addr, unsigned char *op, size_t opsize) {
		_PatchAsm(addr, op, opsize);
	}

	// Replay.cpp
 	void __stdcall Replay_registerLoadReplayCallback(void(__stdcall * callback)(const char * jsonstr)) {
		Replay::registerLoadReplayCallback(callback);
	}

	void __stdcall Replay_registerCreateReplayCallback(const char*(__stdcall * callback)(const char * jsonstr)) {
		Replay::registerCreateReplayCallback(callback);
	}

	void __stdcall Replay_registerHasDataToSaveCallback(int(__stdcall * callback)()) {
		Replay::registerHasDataToSaveCallback(callback);
	}

	//Frontend.cpp
	void __stdcall Frontend_registerChangeScreenCallback(void(__stdcall * callback)(int screen)) {
		Frontend::registerChangeScreenCallback(callback);
	}

	int __stdcall Frontend_getCurrentScreen() {
		return Frontend::getCurrentScreen();
	}

	int __stdcall Frontend_messageBox(const char * message, int a2, int a3) {
		return Frontend::callMessageBox(message, a2, a3);
	}

	//LobbyChat.cpp
	LobbyChatScreens __stdcall LobbyChat_getScreenPointers() {
		static LobbyChatScreens screens;
		screens = {LobbyChat::getLobbyHostScreen(), LobbyChat::getLobbyClientScreen(), LobbyChat::getLobbyOfflineScreen()};
		return screens;
	}

	void __stdcall LobbyChat_lobbyPrint(const char * msg) {
		LobbyChat::lobbyPrint((char*)msg);
	}

	void __stdcall LobbyChat_hostSendGreentext(const char * msg) {
		LobbyChat::sendGreentextMessage(msg);
	}

	void __stdcall LobbyChat_registerClientCommandsCallback(int(__stdcall * callback)(const char * command, const char * args)) {
		LobbyChat::registerClientCommandsCallback(callback);
	}

	void __stdcall LobbyChat_registerHostCommandsCallback(int(__stdcall * callback)(const char * command, const char * args)) {
		LobbyChat::registerHostCommandsCallback(callback);
	}

	//Packets.cpp
	void __stdcall Packets_sendStringToClientConnection(DWORD connection, const char * data) {
		Packets::sendDataToClient_connection(connection, data);
	}

	void __stdcall Packets_sendStringToClientSlot(int slot, const char * data) {
		Packets::sendDataToClient_slot(slot, data);
	}

	void __stdcall Packets_sendStringToHost(const char * data) {
		Packets::sendDataToHost(data);
	}

	int __stdcall Packets_hostBroadcastData(unsigned char * data, size_t size) {
		return Packets::callHostBroadcastData(data, size);
	}

	void __stdcall Packets_registerHostPacketHandler(int(__stdcall * callback)(DWORD HostThis, int slot, unsigned char * packet, size_t size)) {
		Packets::registerHostPacketHandlerCallback(callback);
	}

	void __stdcall Packets_registerHostInternalPacketHandler(int(__stdcall * callback)(DWORD connection, unsigned char * packet, size_t size)) {
		Packets::registerHostInteralPacketHandlerCallback(callback);
	}

	void __stdcall Packets_registerClientPacketHandler(int(__stdcall * callback)(DWORD ClientThis, unsigned char * packet, size_t size)) {
		Packets::registerClientPacketHandlerCallback(callback);
	}

	int __stdcall Packets_isHost() {
		return Packets::isHost();
	}

	int __stdcall Packets_isClient() {
		return Packets::isClient();
	}

	//WaLibc.cpp
	void* __stdcall WaLibc_malloc(size_t size) {
		return WaLibc::waMalloc(size);
	}

	void __stdcall WaLibc_free(void * ptr) {
		WaLibc::waFree(ptr);
	}

	//W2App.cpp
	W2AppObjects __stdcall W2App_getGlobalObjectPointers() {
		static W2AppObjects objects;
		objects = {
			W2App::getAddrDdGame(), W2App::getAddrDdDisplay(), W2App::getAddrDsSound(), W2App::getAddrDdKeyboard(), W2App::getAddrDdMouse(),
			W2App::getAddrWavCdRom(), W2App::getAddrWsGameNet(), W2App::getAddrW2Wrapper(), W2App::getAddrGameinfoObject(), W2App::getAddrGameGlobal()
		};
		return objects;
	}

	void __stdcall W2App_registerConstructGameGlobalCallback(void(__stdcall * callback)(DWORD GameGlobal)) {
		W2App::registerConstructGameGlobalCallback(callback);
	}

	void __stdcall W2App_registerDestroyGameGlobalCallback(void(__stdcall * callback)()) {
		W2App::registerDestroyGameGlobalCallback(callback);
	}

	//Scheme.cpp
	SchemeInfo __stdcall Scheme_getSchemeVersionAndSize() {
		static SchemeInfo ret;
		auto info = Scheme::getSchemeVersionAndSize();
		ret.version = info.first;
		ret.size = info.second;
		return ret;
	}

	DWORD __stdcall Scheme_getSchemeAddr() {
		return Scheme::getAddrSchemeStruct();
	}

	void __stdcall Scheme_setBuiltinScheme(int id) {
		Scheme::callSetBuiltinScheme(id);
	}

	int __stdcall Scheme_setWscScheme(const char * path, int a2) {
		bool ret;
		Scheme::hookSetWscScheme(Scheme::getAddrSchemeStruct(), (char*)path, a2, &ret);
		return ret;
	}

	void __stdcall Scheme_setSchemeName(const char * name) {
		Scheme::setSchemeName(name);
	}

	int __stdcall Scheme_sendCurrentSchemeToPlayers() {
		auto hostscreen = LobbyChat::getLobbyHostScreen();
		if(Packets::isHost() && hostscreen) {
			Scheme::origSendWscScheme(hostscreen, 0);
			return 1;
		}
		return 0;
	}

	void __stdcall Scheme_refreshLobbyDisplay() {
		Scheme::callRefreshOfflineMultiplayerSchemeDisplay();
		Scheme::callRefreshOnlineMultiplayerSchemeDisplay();
	}

	//MapGenerator.cpp
	void __stdcall MapGenerator_registerOnMapResetCallback(void(__stdcall * callback)(int reason)) {
		MapGenerator::registerOnMapResetCallback(callback);
	}
	void __stdcall MapGenerator_registerOnExitMapEditorCallback(void(__stdcall * callback)()) {
		MapGenerator::registerOnMapEditorExitCallback(callback);
	}

	//Sprites.cpp
	int __stdcall Sprites_checkIfFileExistsInVFS(const char * filename, DWORD vfs) {
		return Sprites::callCheckIfFileExistsInVFS(filename, vfs);
	}

	DWORD __stdcall Sprites_loadSpriteFromVFS(DWORD DD_Display, int palette, int index, int a4, DWORD vfs, const char *filename) {
		return Sprites::origLoadSpriteFromVFS(DD_Display, 0, palette, index, a4, vfs, filename);
	}

	DWORD __stdcall Sprites_loadSpriteFromTerrain(DWORD DD_Display, int palette, int index, DWORD vfs, const char *filename) {
		return Sprites::origLoadSpriteFromTerrain(DD_Display, 0, palette, index, vfs, filename);
	}

	void __stdcall Sprites_registerOnLoadSpriteFromVFSCallback(int(__stdcall * callback)(DWORD DD_Display, int palette, int index, int unk, int vfs, const char *filename)) {
		Sprites::registerOnLoadSpriteFromVFSCallback(callback);
	}

	void __stdcall Sprites_registerOnLoadSpriteFromTerrainCallback(int(__stdcall * callback)(DWORD DD_Display, int palette, int index, int vfs, const char *filename)) {
		Sprites::registerOnLoadSpriteFromTerrainCallback(callback);
	}


}
