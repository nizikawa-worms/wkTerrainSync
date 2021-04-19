
#include "Frontend.h"
#include "Hooks.h"
#include "TerrainList.h"
#include "LobbyChat.h"
#include "Config.h"

void (__stdcall *origChangeScreen)(int screen);
void __stdcall Frontend::hookChangeScreen(int screen) {
	int sesi;
	_asm mov sesi, esi
//	printf("hookChangeScreen: screen: %d\n", screen);
	switch(screen) {
		case 10:	// main screen
		case 11:	// local multiplayer
		case 18:	// main screen when leaving from other screens
		case 21:	// single player
		case 1700:	// network
		case 1702:	// lan network
		case 1703:  // wormnet
//		case 1704: 	// lobby host
//		case 1706:	// lobby client
			TerrainList::getLastTerrainInfo().clear();
			LobbyChat::resetLobbyScreenPtrs();
			break;
		default:
			break;
	}

	_asm mov esi, sesi
	_asm push screen
	_asm call origChangeScreen
}

int (__stdcall * origFrontendMessageBox)(char * message, int a2, int a3);

int Frontend::callMessageBox(char * message, int a2, int a3) {
	if(!Config::isMessageBoxEnabled()) return 0;
	return origFrontendMessageBox(message, a2, a3);
}

void Frontend::install() {
	DWORD addrFrontendChangeScreen = Hooks::scanPattern("FrontendChangeScreen", "\x83\x3D\x00\x00\x00\x00\x00\x53\x8B\x5C\x24\x08\x75\x14\x8B\x46\x3C\xA8\x18\x74\x59\x83\xE0\xEF\x89\x5E\x44\x89\x46\x3C\x5B\xC2\x04\x00\x6A\x00\x8B\xCE\xE8\x00\x00\x00\x00\x8B\x86\x00\x00\x00\x00\x50\x8B\x86\x00\x00\x00\x00\x68\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x33\xC0\x57", "???????xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx????xx????xxx????x????x????xxx");
	origFrontendMessageBox =
			(int (__stdcall *)(char *,int,int))
			Hooks::scanPattern("FrontendMessageBox", "\x55\x8B\xEC\x83\xE4\xF8\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x51\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x83\x3D\x00\x00\x00\x00\x00", "??????xxx????xx????xxxx????xx????x????xx?????");

	Hooks::minhook("FrontendChangeScreen", addrFrontendChangeScreen, (DWORD*)&hookChangeScreen, (DWORD*)&origChangeScreen);
}
