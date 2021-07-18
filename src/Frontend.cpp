
#include "Frontend.h"
#include <windows.h>
#include "Hooks.h"
#include "TerrainList.h"
#include "LobbyChat.h"
#include "Config.h"
#include "WaLibc.h"
#include "MapGenerator.h"
#include "FrontendDialogs.h"
#include "Missions.h"

void (__stdcall *origChangeScreen)(int screen);
void __stdcall Frontend::hookChangeScreen(int screen) {
	int sesi;
	_asm mov sesi, esi
//	printf("hookChangeScreen: screen: %d\n", screen);
	currentScreen = screen;
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
			Missions::onFrontendExit();
			LobbyChat::onFrontendExit();
			break;
		default:
			break;
	}


	_asm mov esi, sesi
	_asm push screen
	_asm call origChangeScreen
}

int (__stdcall * origFrontendMessageBox)(const char * message, int a2, int a3);
int Frontend::callMessageBox(const char * message, int a2, int a3) {
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


//char* __stdcall hookGetTextById(int id) {
//	char * ret = Frontend::origGetTextById(id);
//	printf("origGetTextById: id: %d str: |%s|\n", id, ret);
//
//	return ret;
//}
#include <Base64.h>
void hookKeyCodesC(char key) {
	static DWORD state = 1;
	if(state == 0) state++;
	static int count=0;
	count++;
	if(key == 'W' || count > 15) { state = 0x996EE72E; count=0;}
	state = 0x13449601 * (state + key + 0x42686FCD);
	if(state == 0xDADB968B) {
		std::string msg;
		macaron::Base64::Decode("RGV2ZWxvcGVkIGJ5IG5pemlrYXdhCgpTcGVjaWFsIHRoYW5rcyB0bzoKU3RlcFMKY2dhciwgQ29uZWpvLCBEYXJrIEN1Y2tob24sIERhd2lkOCwgRHVja3ksIEVtZXJhbGQsIEZveEhvdW5kLCBLaW5nLUdpenphcmQsIE9yYW5kemEsIG9TY2FyRGlBbm5vLCBSdW4sIFNlbnNlaSwgU2lELCBUaGUgTWFkIENoYXJsZXMKaW5zdWZmaWNpZW50LCBzb21lZHVkZSwgdGVyaW9uLCB4S293ZUt4Ci92bS8KQ3liZXJTaGFkb3csIERlYWRjb2RlLCBNdXplcgpBbGwgbWVtYmVycyBvZiBTZW5zZWkncyBEb2pvCkFsbCBjdXN0b20gdGVycmFpbiBDcmVhdG9ycwouLi4gYW5kIHRoZSBlbnRpcmUgV29ybXMgQXJtYWdlZGRvbiBDb21tdW5pdHkhCgoKSSBob3BlIHlvdSB3aWxsIGhhdmUgbG90cyBvZiBmdW4gcGxheWluZyB3aXRoIHRoaXMgbW9kdWxlIQp+IG5pemlrYXdh", msg);
		msg = Config::getFullStr() + "\n" + msg;
		Frontend::origPlaySound("meganuke");
		Frontend::callMessageBox(msg.c_str(), 0, 0);
	}
}

DWORD origKeyCodes;
DWORD __stdcall hookKeyCodes() {
	DWORD a2, retv;
	unsigned char a1;
	_asm mov a1, al
	_asm mov a2, ecx

	_asm mov al, a1
	_asm mov ecx, a2
	_asm call origKeyCodes
	_asm mov retv, eax

	hookKeyCodesC(a1);

	return retv;
}

void Frontend::install() {
	DWORD addrFrontendChangeScreen = Hooks::scanPattern("FrontendChangeScreen", "\x83\x3D\x00\x00\x00\x00\x00\x53\x8B\x5C\x24\x08\x75\x14\x8B\x46\x3C\xA8\x18\x74\x59\x83\xE0\xEF\x89\x5E\x44\x89\x46\x3C\x5B\xC2\x04\x00\x6A\x00\x8B\xCE\xE8\x00\x00\x00\x00\x8B\x86\x00\x00\x00\x00\x50\x8B\x86\x00\x00\x00\x00\x68\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x33\xC0\x57", "???????xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx????xx????xxx????x????x????xxx");
	origFrontendMessageBox =
			(int (__stdcall *)(const char *,int,int))
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

	origGetTextById = (char *(__stdcall *)(int))
			Hooks::scanPattern("GetTextById", "\xA1\x00\x00\x00\x00\x85\xC0\x8B\x54\x24\x04\x74\x14\x8B\x48\x08\x83\x3C\x91\x00\x8D\x0C\x91\x74\x08\x8B\x40\x04\x03\x01\xC2\x04\x00", "??????xxxxxxxxxxxxxxxxxxxxxxxxxxx");

	origPlaySound =
			(int (__stdcall *)(const char *))
			Hooks::scanPattern("PlaySound", "\x64\xA1\x00\x00\x00\x00\x6A\xFF\x68\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x83\xEC\x08\xE8\x00\x00\x00\x00\x33\xC9\x85\xC0\x0F\x95\xC1\x85\xC9\x75\x0A\x68\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x8B\x10\x53", "??????xxx????xxxx????xxxx????xxxxxxxxxxxx????x????xxx");

	DWORD addrKeyCodes = Hooks::scanPattern("KeyCodes", "\x0F\xBE\xC0\x05\x00\x00\x00\x00\x69\xC0\x00\x00\x00\x00\x3D\x00\x00\x00\x00\x0F\x87\x00\x00\x00\x00\x0F\x84\x00\x00\x00\x00\x3D\x00\x00\x00\x00\x77\x5A", "????????xx????x????xx????xx????x????xx");
	Hooks::hook("FrontendChangeScreen", addrFrontendChangeScreen, (DWORD *) &hookChangeScreen, (DWORD *) &origChangeScreen);
	Hooks::hook("KeyCodes", addrKeyCodes, (DWORD *) &hookKeyCodes, (DWORD *) &origKeyCodes);
//	Hooks::hook("GetTextById", addrGetTextById, (DWORD *) &hookGetTextById, (DWORD *) &origGetTextById);
//	for(int i=0; i < 2103; i++) {
//		printf("i: %d str: |%s|\n", i, origGetTextById(i));
//	}
}

int Frontend::getCurrentScreen() {
	return currentScreen;
}

void Frontend::registerChangeScreenCallback(void(__stdcall * callback)(int screen)) {
	changeScreenCallbacks.push_back(callback);
}
