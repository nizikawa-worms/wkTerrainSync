
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
#include "Debugf.h"

void (__stdcall *origFrontendChangeScreen)(int screen);
void __stdcall Frontend::hookFrontendChangeScreen(int screen) {
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
		case 25: // intro
			screen = 18;
			break;
		default:
			break;
	}


	_asm mov esi, sesi
	_asm push screen
	_asm call origFrontendChangeScreen
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
		macaron::Base64::Decode("RGV2ZWxvcGVkIGJ5IG5pemlrYXdhCgpTcGVjaWFsIHRoYW5rcyB0bzoKY2dhciwgQ29uZWpvLCBEYXJrIEN1Y2tob24sIERhd2lkOCwgRHVja3ksIEVtZXJhbGQsIEZveEhvdW5kLCBLaW5nLUdpenphcmQsIE9yYW5kemEsIG9TY2FyRGlBbm5vLCBSdW4sIFNlbnNlaSwgU2lELCBUaGUgTWFkIENoYXJsZXMsIFN0ZXBTClRoZSBCaWcgR3V5cwpDeWJlclNoYWRvdywgRGVhZGNvZGUsIE11emVyCkFsbCBtZW1iZXJzIG9mIFNlbnNlaSdzIERvam8KQWxsIGN1c3RvbSB0ZXJyYWluIENyZWF0b3JzCi4uLiBhbmQgdGhlIGVudGlyZSBXb3JtcyBBcm1hZ2VkZG9uIENvbW11bml0eSE=", msg);
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

DWORD origSetStaticTextValue;
void Frontend::callSetStaticTextValue(CWnd * control, const char * text) {
	_asm mov esi, control
	_asm push text
	_asm call origSetStaticTextValue
}

int (__fastcall *origGetLocalizedCString)(int a1, int a2, char** a3);
int __fastcall Frontend::hookGetLocalizedCString(int a1, int id_a2, char** a3) {
	if(id_a2) {
		if(id_a2 == hostMapHelpTextID || id_a2 == clientMapHelpTextID) {
			auto & terrain = TerrainList::getLastTerrainInfo();
			if(terrain) {
				char scale[64] = "";
				if(MapGenerator::getScaleXIncrements() || MapGenerator::getScaleYIncrements()) {
					_snprintf_s(scale, _TRUNCATE, " [%.01fx%.01f]", MapGenerator::getEffectiveScaleX(), MapGenerator::getEffectiveScaleY());
				}

				char buff[256];
				_snprintf_s(buff, _TRUNCATE,"Current terrain: %s %s%s%s",
							terrain->name.c_str(),
							terrain->custom ? "(Custom)" : "",
							scale,
							id_a2 == hostMapHelpTextID ? "\nCtrl+click to generate a new map with a custom terrain, Alt+click to generate a new map with a standard terrain" : "");
				WaLibc::CStringBufferFromString(a3, 0, buff, strlen(buff));
				return 1;
			} else {
				if(id_a2 == clientMapHelpTextID) {id_a2 = 0;}
			}
		}
	}
	return origGetLocalizedCString(a1, id_a2, a3);
}

int (__fastcall *origMapThumbnailOnMouseMove)(CWnd *This, int EDX, int a2, int a3, int a4);
int __fastcall Frontend::hookMapThumbnailOnMouseMove(CWnd* This, int EDX, int a2, int a3, int a4) {
	DWORD * stringid = ((DWORD *)This + 0xA587);
	if(!*stringid) {*stringid = clientMapHelpTextID;} //set help text of client's map thumbnail
	return origMapThumbnailOnMouseMove(This, EDX, a2, a3, a4);
}

void Frontend::refreshMapThumbnailHelpText(CWnd * map) {
	if(*addrCurrentHelpText == hostMapHelpTextID || *addrCurrentHelpText == clientMapHelpTextID) {
		*addrCurrentHelpText = 0;
		hookMapThumbnailOnMouseMove(map, 0, 0, 0, 0);
	}
}


void Frontend::install() {
	DWORD addrFrontendChangeScreen = _ScanPattern("FrontendChangeScreen", "\x83\x3D\x00\x00\x00\x00\x00\x53\x8B\x5C\x24\x08\x75\x14\x8B\x46\x3C\xA8\x18\x74\x59\x83\xE0\xEF\x89\x5E\x44\x89\x46\x3C\x5B\xC2\x04\x00\x6A\x00\x8B\xCE\xE8\x00\x00\x00\x00\x8B\x86\x00\x00\x00\x00\x50\x8B\x86\x00\x00\x00\x00\x68\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x33\xC0\x57", "???????xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx????xx????xxx????x????x????xxx");
	origFrontendMessageBox =
		(int (__stdcall *)(const char *,int,int))
		_ScanPattern("FrontendMessageBox", "\x55\x8B\xEC\x83\xE4\xF8\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x51\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x83\x3D\x00\x00\x00\x00\x00", "??????xxx????xx????xxxx????xx????x????xx?????");

	origAfxSetupComboBox =
		(int (__stdcall *)(void))
		_ScanPattern("AfxSetupComboBox", "\x53\x6A\x01\x33\xDB\x53\x56\xE8\x00\x00\x00\x00\x89\x9E\x00\x00\x00\x00\xC7\x06\x00\x00\x00\x00\xC7\x46\x00\x00\x00\x00\x00\xC7\x05\x00\x00\x00\x00\x00\x00\x00\x00\x88\x9E\x00\x00\x00\x00\x88\x9E\x00\x00\x00\x00", "??????xx????xx????xx????xx?????xx????????xx????xx????");

	origAfxSetupStaticText =
		(CWnd *(__stdcall *)(CWnd *))
		_ScanPattern("AfxSetupStaticText", "\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x51\x56\x8B\x74\x24\x18\x57\x8B\xCE\xE8\x00\x00\x00\x00\xC7\x06\x00\x00\x00\x00\x8D\x7E\x54", "???????xx????xxxx????xxxxxxxxxx????xx????xxx");

	origAddEntryToComboBox =
		(int (__fastcall *)(char *,int,CWnd *,int))
		_ScanPattern("AddEntryToComboBox", "\x55\x8B\xEC\x83\xE4\xF8\x6A\xFF\x64\xA1\x00\x00\x00\x00\x68\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x83\xEC\x30\x53\x8B\x5D\x08\x56\x57\x6A\x0C\x8B\xF1\xE8\x00\x00\x00\x00\x8B\xF8", "??????xxxx????x????xxxx????xxxxxxxxxxxxxx????xx");

	origWaDDX_Control =
		(void (__stdcall *)(CDataExchange *,int,CWnd*))
		_ScanPattern("WaDDX_Control", "\x55\x8B\xEC\x57\x8B\x7D\x10\x83\x7F\x20\x00\x75\x77\x8B\xCF\xE8\x00\x00\x00\x00\x85\xC0\x75\x6C\x53\x8B\x5D\x0C", "??????xxxxxxxxxx????xxxxxxxx");

	origGetTextById =
		(char *(__stdcall *)(int))
		_ScanPattern("GetTextById", "\xA1\x00\x00\x00\x00\x85\xC0\x8B\x54\x24\x04\x74\x14\x8B\x48\x08\x83\x3C\x91\x00\x8D\x0C\x91\x74\x08\x8B\x40\x04\x03\x01\xC2\x04\x00", "??????xxxxxxxxxxxxxxxxxxxxxxxxxxx");

	origPlaySound =
		(int (__stdcall *)(const char *))
		Hooks::scanPattern("PlaySound", "\x64\xA1\x00\x00\x00\x00\x6A\xFF\x68\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x83\xEC\x08\xE8\x00\x00\x00\x00\x33\xC9\x85\xC0\x0F\x95\xC1\x85\xC9\x75\x0A\x68\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x8B\x10\x53", "??????xxx????xxxx????xxxx????xxxxxxxxxxxx????x????xxx");

	DWORD addrGetLocalizedCString = _ScanPattern("GetLocalizedCString", "\x8B\x0D\x00\x00\x00\x00\x85\xC9\x74\x13\x8B\x41\x08\x83\x3C\x90\x00\x8D\x04\x90\x74\x07\x8B\x49\x04\x03\x08\xEB\x24\x8B\x0D\x00\x00\x00\x00\x85\xC9\x74\x13", "??????xxxxxxxxxxxxxxxxxxxxxxxxx????xxxx");
//	setHelpText =
//		(int (__fastcall *)(CWnd *,int,int,int,int))
//		_ScanPattern("SetHelpText", "\x55\x8B\xEC\x83\xE4\xF8\x64\xA1\x00\x00\x00\x00\x6A\xFF\x68\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x83\xEC\x08\x53\x56\x57\x68\x00\x00\x00\x00\x8B\xD9\xE8\x00\x00\x00\x00\x8B\xF0\x85\xF6", "??????xx????xxx????xxxx????xxxxxxx????xxx????xxxx");

	origSetStaticTextValue = _ScanPattern("SetStaticTextValue", "\x64\xA1\x00\x00\x00\x00\x6A\xFF\x68\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x8B\x44\x24\x10\x50\x8D\x4C\x24\x14\xE8\x00\x00\x00\x00\x8D\x54\x24\x10", "??????xxx????xxxx????xxxxxxxxxx????xxxx");
	DWORD addrKeyCodes = _ScanPattern("KeyCodes", "\x0F\xBE\xC0\x05\x00\x00\x00\x00\x69\xC0\x00\x00\x00\x00\x3D\x00\x00\x00\x00\x0F\x87\x00\x00\x00\x00\x0F\x84\x00\x00\x00\x00\x3D\x00\x00\x00\x00\x77\x5A", "????????xx????x????xx????xx????x????xx");

	DWORD addrMapThumbnailOnMouseMove = _ScanPattern("MapThumbnailOnMouseMove", "\x55\x8B\xEC\x83\xE4\xF8\xA1\x00\x00\x00\x00\x56\x8B\xF1\x3B\x86\x00\x00\x00\x00\x57\x74\x2D\x8B\x4E\x20\x51\xFF\x15\x00\x00\x00\x00\x50\xE8\x00\x00\x00\x00", "??????x????xxxxx????xxxxxxxxx????xx????");
	addrCurrentHelpText = *(DWORD**)(addrMapThumbnailOnMouseMove + 0x7);
	CWndFromHandle = (CWnd *(__stdcall *)(int))(addrMapThumbnailOnMouseMove + 0x27 + *(DWORD*)(addrMapThumbnailOnMouseMove + 0x23));
	CWndGetDlgItem = (CWnd *(__fastcall *)(CWnd *,int,int))(addrMapThumbnailOnMouseMove + 0x33 + *(DWORD*)(addrMapThumbnailOnMouseMove + 0x2F));
	debugf("addrCWndFromHandle: 0x%X addrCWndGetDlgItem: 0x%X\n", CWndFromHandle, CWndGetDlgItem);

	_HookDefault(FrontendChangeScreen);
	_HookDefault(KeyCodes);
	_HookDefault(GetLocalizedCString);
	_HookDefault(MapThumbnailOnMouseMove);

//	Hookf("GetTextById", addrGetTextById, (DWORD *) &hookGetTextById, (DWORD *) &origGetTextById);
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

