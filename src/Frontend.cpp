
#include "Frontend.h"
#include <windows.h>
#include "Hooks.h"
#include "TerrainList.h"
#include "LobbyChat.h"
#include "Config.h"
#include "WaLibc.h"
#include "MapGenerator.h"
#include "FrontendDialogs.h"

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
			TerrainList::onFrontendExit();
			MapGenerator::onFrontendExit();
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

std::pair<int, int> Frontend::scalePosition(int x, int y, int realWidth, int realHeight) {
	double widthRatio = realWidth / 640.0;
	double heightRatio = realHeight / 480.0;
	double scaleRatio = max(widthRatio, heightRatio);
	int startX = max((realWidth - scaleRatio * 640.0) / 2, 0);
	int startY = max((realHeight - scaleRatio * 480.0) / 2, 0);
//	printf("realWidth: %d realHeight: %d\n", realWidth, realHeight);
//	printf("widthRatio: %lf heightRatio: %lf scaleRatio: %lf startX: %d startY: %d\n", widthRatio, heightRatio, scaleRatio, startX, startY);
	return {(int)(startX + widthRatio*x), (int)(startY + heightRatio*y)};
}

int Frontend::scaleSize(int value, int realWidth, int realHeight) {
	double widthRatio = realWidth / 640.0;
	double heightRatio = realHeight / 480.0;
	double scaleRatio = max(widthRatio, heightRatio);
	return value * scaleRatio;
}

int (__stdcall *origAfxSetupComboBox)();
int Frontend::setupComboBox(CWnd * box) {
	int retv;
	_asm mov esi, box
	_asm call origAfxSetupComboBox
	_asm mov retv, eax
	return retv;
}


void Frontend::install() {
	DWORD addrFrontendChangeScreen = Hooks::scanPattern("FrontendChangeScreen", "\x83\x3D\x00\x00\x00\x00\x00\x53\x8B\x5C\x24\x08\x75\x14\x8B\x46\x3C\xA8\x18\x74\x59\x83\xE0\xEF\x89\x5E\x44\x89\x46\x3C\x5B\xC2\x04\x00\x6A\x00\x8B\xCE\xE8\x00\x00\x00\x00\x8B\x86\x00\x00\x00\x00\x50\x8B\x86\x00\x00\x00\x00\x68\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x33\xC0\x57", "???????xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx????xx????xxx????x????x????xxx");
	origFrontendMessageBox =
			(int (__stdcall *)(char *,int,int))
			Hooks::scanPattern("FrontendMessageBox", "\x55\x8B\xEC\x83\xE4\xF8\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x51\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x83\x3D\x00\x00\x00\x00\x00", "??????xxx????xx????xxxx????xx????x????xx?????");

	origAfxSetupComboBox =
		(int (__stdcall *)(void))
		Hooks::scanPattern("AfxSetupComboBox", "\x53\x6A\x01\x33\xDB\x53\x56\xE8\x00\x00\x00\x00\x89\x9E\x00\x00\x00\x00\xC7\x06\x00\x00\x00\x00\xC7\x46\x00\x00\x00\x00\x00\xC7\x05\x00\x00\x00\x00\x00\x00\x00\x00\x88\x9E\x00\x00\x00\x00\x88\x9E\x00\x00\x00\x00", "??????xx????xx????xx????xx?????xx????????xx????xx????");

	origAfxSetupStaticText =
		(CWnd *(__stdcall *)(CWnd *))
		Hooks::scanPattern("AfxSetupStaticText", "\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x51\x56\x8B\x74\x24\x18\x57\x8B\xCE\xE8\x00\x00\x00\x00\xC7\x06\x00\x00\x00\x00\x8D\x7E\x54", "???????xx????xxxx????xxxxxxxxxx????xx????xxx");

	origAddEntryToComboBox =
		(int (__fastcall *)(char *,int,CWnd *,int))
		Hooks::scanPattern("AddEntryToComboBox", "\x55\x8B\xEC\x83\xE4\xF8\x6A\xFF\x64\xA1\x00\x00\x00\x00\x68\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x83\xEC\x30\x53\x8B\x5D\x08\x56\x57\x6A\x0C\x8B\xF1\xE8\x00\x00\x00\x00\x8B\xF8", "??????xxxx????x????xxxx????xxxxxxxxxxxxxx????xx");

	origWaDDX_Control =
		(void (__stdcall *)(CDataExchange *,int,CWnd*))
		Hooks::scanPattern("WaDDX_Control", "\x55\x8B\xEC\x57\x8B\x7D\x10\x83\x7F\x20\x00\x75\x77\x8B\xCF\xE8\x00\x00\x00\x00\x85\xC0\x75\x6C\x53\x8B\x5D\x0C", "??????xxxxxxxxxx????xxxxxxxx");

	Hooks::hook("FrontendChangeScreen", addrFrontendChangeScreen, (DWORD *) &hookChangeScreen, (DWORD *) &origChangeScreen);
}
