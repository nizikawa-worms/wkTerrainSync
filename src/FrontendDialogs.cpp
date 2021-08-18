#include "FrontendDialogs.h"
#include <windows.h>
#include <CommCtrl.h>
#include "Hooks.h"
#include "WaLibc.h"
#include "Frontend.h"
#include "LobbyChat.h"
#include "MapGenerator.h"
#include "Config.h"
#include "Debugf.h"

void (__fastcall *origLocalMultiplayerDDX_Control)(CWnd* This, int EDX, CDataExchange *a2);
void __fastcall FrontendDialogs::hookLocalMultiplayerDDX_Control(CWnd* This, int EDX, CDataExchange *a2) {
	origLocalMultiplayerDDX_Control(This, EDX, a2);
	CWnd * mapList = (CWnd*)((int)This + 0x158);
	createTerrainSizeComboBoxes(This, mapList, a2, 175, 124);
}

void (__fastcall *origLocalMultiplayerEndscreenDDX_Control)(CWnd* This, int EDX, CDataExchange *a2);
void __fastcall FrontendDialogs::hookLocalMultiplayerEndscreenDDX_Control(CWnd* This, int EDX, CDataExchange *a2) {
	origLocalMultiplayerEndscreenDDX_Control(This, EDX, a2);
	CWnd * mapList = (CWnd*)((int)This + 0x29768);
	createTerrainSizeComboBoxes(This, mapList, a2, 175, 159);
}


void (__fastcall *origLobbyHostScreenDDX_Control)(CWnd* This, int EDX, CDataExchange *a2);
void __fastcall FrontendDialogs::hookLobbyHostScreenDDX_Control(CWnd* This, int EDX, CDataExchange *a2) {
	origLobbyHostScreenDDX_Control(This, EDX, a2);
	CWnd * mapList = (CWnd*)((int)This + 0x39B50);
	createTerrainSizeComboBoxes(This, mapList, a2, 516, 102, -10);
}

void (__fastcall *origLobbyHostEndScreenWrapperDDX_Control)(CWnd* This, int EDX, CDataExchange *a2);
void __fastcall FrontendDialogs::hookLobbyHostEndScreenWrapperDDX_Control(CWnd* This, int EDX, CDataExchange *a2) {
	origLobbyHostEndScreenWrapperDDX_Control(This, EDX, a2);
	CWnd * mapList = (CWnd*)((int)This + 0x39B50);
	createTerrainSizeComboBoxes(This, mapList, a2, 516, 102);
}


LRESULT (__stdcall *origWidthWndProc)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT __stdcall FrontendDialogs::hookWidthWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if(uMsg == 0xC) {
//		char * msg = (char*)lParam;
		int position = *(int *)((DWORD)comboWidth + 0x414);
		if(position >=0 )
			MapGenerator::setScaleX(position, false);
	}
	return origWidthWndProc(hWnd, uMsg, wParam, lParam);
}

LRESULT (__stdcall *origHeightWndProc)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT __stdcall FrontendDialogs::hookHeightWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if(uMsg == 0xC) {
//		char * msg = (char*)lParam;
		int position = *(int *)((DWORD)comboHeight + 0x414);
		if(position >=0 )
			MapGenerator::setScaleY(position, false);
	}
	return origHeightWndProc(hWnd, uMsg, wParam, lParam);
}


void FrontendDialogs::createTerrainSizeComboBoxes(CWnd *window, CWnd *mapList, CDataExchange *dataExchange, int startX, int startY, int moveMapCombo) {
	if(comboWidth || comboHeight) {
		return;
	}
	comboMoved = false;
	moveMapComboLeft = moveMapCombo;
	HWND windowHwnd = *(HWND*)((DWORD) window + 0x20);
	RECT windowRect;
	GetWindowRect(windowHwnd, &windowRect);
	windowWidth = windowRect.right - windowRect.left;
	windowHeight = windowRect.bottom - windowRect.top;

	RECT mapListRect;
	HWND mapListHwnd = *(HWND*)((DWORD)mapList + 0x20);
	GetWindowRect(mapListHwnd, &mapListRect);
	int mapListWidth = mapListRect.right - mapListRect.left;
	int mapListHeight = mapListRect.bottom - mapListRect.top;

	mapListWidth *= 0.5;
	SetWindowPos(mapListHwnd,NULL, mapListRect.left, mapListRect.top, mapListWidth, mapListHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	GetWindowRect(mapListHwnd, &mapListRect);

	// at this moment the maplist is placed offscreen, so just manually specify XY pos
	auto scaled = Frontend::scalePosition(startX, startY, windowWidth, windowHeight);
	int comboWidthXpos = scaled.first;
	int comboWidthYpos = scaled.second;
	int comboWidthXsize = Frontend::scaleSize(60, windowWidth, windowHeight);
	int comboWidthYsize = mapListHeight;

	int comboHeightXpos = comboWidthXpos + comboWidthXsize + Frontend::scaleSize(5, windowWidth, windowHeight);
	int comboHeightYpos = comboWidthYpos;
	int comboHeightXsize = comboWidthXsize;
	int comboHeightYsize = mapListHeight;

	comboWidthHwnd = CreateWindow(WC_EDIT, TEXT("1.0x"),
									 ES_LEFT | ES_AUTOHSCROLL | ES_READONLY | WS_CHILD | WS_VISIBLE,
									   comboWidthXpos, comboWidthYpos, comboWidthXsize, comboWidthYsize,
									 windowHwnd, (HMENU)comboWidthID, GetModuleHandle(NULL), NULL);
	comboWidth = (CWnd*)WaLibc::waMalloc(1068);
	Frontend::setupComboBox(comboWidth);
	Frontend::origWaDDX_Control(dataExchange, comboWidthID, comboWidth);
	origWidthWndProc = (LRESULT (__stdcall  *)(HWND, UINT,WPARAM,LPARAM)) GetWindowLongPtrA(comboWidthHwnd, GWL_WNDPROC);
	SetWindowLongPtr(comboWidthHwnd, GWL_WNDPROC, (LONG)hookWidthWndProc);
	SetWindowPos(comboWidthHwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	comboHeightHwnd = CreateWindow(WC_EDIT, TEXT("1.0x"),
									   ES_LEFT | ES_AUTOHSCROLL | ES_READONLY | WS_CHILD | WS_VISIBLE,
										comboHeightXpos, comboHeightYpos, comboHeightXsize, comboHeightYsize,
									   windowHwnd, (HMENU)comboHeightID, GetModuleHandle(NULL), NULL);
	comboHeight = (CWnd*)WaLibc::waMalloc(1068);
	Frontend::setupComboBox(comboHeight);
	Frontend::origWaDDX_Control(dataExchange, comboHeightID, comboHeight);
	origHeightWndProc = (LRESULT (__stdcall  *)(HWND, UINT,WPARAM,LPARAM)) GetWindowLongPtrA(comboHeightHwnd, GWL_WNDPROC);
	SetWindowLongPtr(comboHeightHwnd, GWL_WNDPROC, (LONG)hookHeightWndProc);
	SetWindowPos(comboHeightHwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	for(int i=0; i <= (MapGenerator::scaleMax - 1) / MapGenerator::scaleIncrement; i++) {
		char buff[32];
		float value = 1.0 + i * MapGenerator::scaleIncrement;
		_snprintf_s(buff, _TRUNCATE, "%.01fx", value);
		Frontend::origAddEntryToComboBox(buff, 0, comboWidth, 1000);
		Frontend::origAddEntryToComboBox(buff, 0, comboHeight, 1000);
	}
	resetComboBoxesText();
}

void FrontendDialogs::destroyTerrainSizeComboBoxes() {
	if(comboWidth) {
		(*(void (__thiscall **)(CWnd*))(*(DWORD *)comboWidth + 0x60))(comboWidth); //		comboWidth->DestroyWindow();
		DestroyWindow(comboWidthHwnd);
		comboWidth = nullptr;
		comboWidthHwnd = NULL;
	}
	if(comboHeight) {
		(*(void (__thiscall **)(CWnd*))(*(DWORD *)comboHeight + 0x60))(comboHeight); //		comboHeight->DestroyWindow();
		DestroyWindow(comboHeightHwnd);
		comboHeight = nullptr;
		comboHeightHwnd = NULL;
	}
}

void FrontendDialogs::resetComboBoxesText() {
	if(comboWidth) {
		HWND widthHwnd = *(HWND*)((DWORD)comboWidth + 0x20);
		float scale = MapGenerator::getEffectiveScaleX();
		char buff[32];
		_snprintf_s(buff, _TRUNCATE, "%.01fx", scale);
		*(int *)((DWORD)comboWidth + 0x414) = -1;
		SetWindowTextA(widthHwnd, buff);
	}
	if(comboHeight) {
		HWND heightHwnd = *(HWND*)((DWORD)comboHeight + 0x20);
		float scale = MapGenerator::getEffectiveScaleY();
		char buff[32];
		_snprintf_s(buff, _TRUNCATE, "%.01fx", scale);
		*(int *)((DWORD)comboHeight + 0x414) = -1;
		SetWindowTextA(heightHwnd, buff);
	}
}

int (__stdcall *origAddDefaultLevelsToCombo)();
int __stdcall FrontendDialogs::hookAddDefaultLevelsToCombo() {
	int sedi, retv;
	_asm mov sedi, edi

	if(!comboMoved) {
		HWND mapHwnd = *(HWND*)((DWORD) sedi + 0x20);
		RECT mapRect;
		GetWindowRect(mapHwnd, &mapRect);
		MapWindowPoints(HWND_DESKTOP, GetParent(mapHwnd), (LPPOINT) &mapRect, 2);

		if(moveMapComboLeft) {
			SetWindowPos(mapHwnd, HWND_TOP, mapRect.left + Frontend::scaleSize(moveMapComboLeft, windowWidth, windowHeight), mapRect.top, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOSENDCHANGING);
			moveMapComboLeft = 0;
			GetWindowRect(mapHwnd, &mapRect);
			MapWindowPoints(HWND_DESKTOP, GetParent(mapHwnd), (LPPOINT) &mapRect, 2);
		}

		int mapWidth = mapRect.right - mapRect.left;
		int mapHeight = mapRect.bottom - mapRect.top;

		HWND comboWidthHwnd = *(HWND*)((DWORD) comboWidth + 0x20);
		HWND comboHeightHwnd = *(HWND*)((DWORD) comboHeight + 0x20);

		int combowidth = mapWidth / 2;
		int y = mapRect.top;
		int x1 = mapRect.right + Frontend::scaleSize(5, windowWidth, windowHeight);
		int x2 = x1 + combowidth + Frontend::scaleSize(5, windowWidth, windowHeight);
		SetWindowPos(comboWidthHwnd, HWND_TOP, x1, y, combowidth, mapHeight, 0);
		SetWindowPos(comboHeightHwnd, HWND_TOP, x2, y, combowidth, mapHeight, 0);
		comboMoved = true;
	}

	_asm mov edi, sedi
	_asm call origAddDefaultLevelsToCombo
	_asm mov retv, eax

	return retv;
}


void FrontendDialogs::install() {
	DWORD addrAddDefaultLevelsToCombo = _ScanPattern("AddDefaultLevelsToCombo", "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x08\x53\x56\xBE\x00\x00\x00\x00\xC7\x44\x24\x00\x00\x00\x00\x00\xEB\x06\x8D\x9B\x00\x00\x00\x00\x0F\xB6\x5E\x09\x68\x00\x00\x00\x00\x57", "??????xxxxxx????xxx?????xxxxxxxxxxxxx????x");
	DWORD addrCreateLocalMultiplayerDialog = _ScanPattern("CreateLocalMultiplayerDialog", "\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x83\xEC\x24\x55\x8B\x6C\x24\x38\x56\x57\x33\xFF\x57\x68\x00\x00\x00\x00\x55\xE8\x00\x00\x00\x00\x89\xBD\x00\x00\x00\x00\x89\xBD\x00\x00\x00\x00\x89\x7C\x24\x38", "???????xx????xxxx????xxxxxxxxxxxxxx????xx????xx????xx????xxxx");
	DWORD * LocalMultiplayerCDialogVTable = *(DWORD**)(addrCreateLocalMultiplayerDialog + 0x46);
	DWORD addrLocalMultiplayerDDX_Control = LocalMultiplayerCDialogVTable[61];
	debugf("LocalMultiplayerCDialogVTable: 0x%X DDX_Control: 0x%X\n", (DWORD)LocalMultiplayerCDialogVTable, (DWORD)addrLocalMultiplayerDDX_Control);

	DWORD addrConstructLocalMultiplayerEndscreenDialog = _ScanPattern("ConstructLocalMultiplayerEndscreenDialog", "\x64\xA1\x00\x00\x00\x00\x6A\xFF\x68\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x53\x56\x57\x8B\x7C\x24\x1C\x33\xDB\x53\x68\x00\x00\x00\x00\x57\xE8\x00\x00\x00\x00\x89\x9F\x00\x00\x00\x00\x89\x9F\x00\x00\x00\x00\x6A\x01", "??????xxx????xxxx????xxxxxxxxxxx????xx????xx????xx????xx");
	DWORD * LocalMultiplayerEndscreenDialogVTable = *(DWORD**)(addrConstructLocalMultiplayerEndscreenDialog + 0x45);
	DWORD addrLocalMultiplayerEndscreenDDX_Control = LocalMultiplayerEndscreenDialogVTable[61];
	debugf("LocalMultiplayerEndscreenDialogVTable: 0x%X DDX_Control: 0x%X\n", (DWORD)LocalMultiplayerEndscreenDialogVTable, (DWORD)LocalMultiplayerEndscreenDialogVTable);

	DWORD addrConstructLobbyHostScreen = LobbyChat::getAddrConstructLobbyHostScreen();
	DWORD * LobbyHostScreenVTable = *(DWORD**)(addrConstructLobbyHostScreen + 0x41);
	DWORD addrLobbyHostScreenDDX_Control = LobbyHostScreenVTable[61];
	debugf("LobbyHostScreenVTable: 0x%X DDX_Control: 0x%X\n", (DWORD)LobbyHostScreenVTable, (DWORD)addrLobbyHostScreenDDX_Control);

	DWORD addrConstructLobbyHostEndScreenWrapper = LobbyChat::getAddrConstructLobbyHostEndScreenWrapper();
	DWORD * LobbyHostEndScreenWrapperVTable = *(DWORD**)(addrConstructLobbyHostEndScreenWrapper + 0x40);
	DWORD addrLobbyHostEndScreenWrapperDDX_Control = LobbyHostEndScreenWrapperVTable[61];
	debugf("LobbyHostEndScreenWrapperVTable: 0x%X DDX_Control: 0x%X\n", (DWORD)LobbyHostEndScreenWrapperVTable, (DWORD)addrLobbyHostEndScreenWrapperDDX_Control);

	_HookDefault(LocalMultiplayerDDX_Control);
	_HookDefault(LobbyHostScreenDDX_Control);
	_HookDefault(LobbyHostEndScreenWrapperDDX_Control);
	_HookDefault(LocalMultiplayerEndscreenDDX_Control);
	_HookDefault(AddDefaultLevelsToCombo);
}
