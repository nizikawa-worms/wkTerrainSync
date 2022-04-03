
#include "Scheme.h"
#include "Hooks.h"
#include "Missions.h"
#include "Packets.h"
#include "LobbyChat.h"
#include "Utils.h"
#include "Config.h"
#include "WaLibc.h"
#include "Frontend.h"
#include "Debugf.h"
#include <Windows.h>

namespace fs = std::filesystem;

void Scheme::dumpSchemeFromResources(int id, std::filesystem::path path) {
//	if(std::filesystem::exists(path)) return;
	try {
		HMODULE hModule = NULL;
		HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(id), "SCHEMES");
		if(!hResource) throw std::runtime_error("FindResource failed");
		HGLOBAL hMemory = LoadResource(hModule, hResource);
		if(!hMemory) throw std::runtime_error("LoadResource failed");
		DWORD dwSize = SizeofResource(hModule, hResource);
		if(!dwSize) throw std::runtime_error("SizeofResource failed");
		LPVOID lpAddress = LockResource(hMemory);
		if(!lpAddress) throw std::runtime_error("LockResource failed");

		FILE * pf = fopen((Config::getWaDir() / path).string().c_str(), "wb");
		if(pf) {
			fwrite(lpAddress, dwSize, 1, pf);
			fclose(pf);
		} else throw std::runtime_error("Failed to save .wsc");
	} catch (std::exception & e) {
		debugf("dumping resource: %d failed, reason: %s\n", id, e.what());
	}
}

int __stdcall Scheme::hookSetWscScheme(DWORD schemestruct, char * path, char flag, bool * out) {
//	printf("hookSetWscScheme struct: 0x%X path: %s flag: %d out: 0x%X\n", schemestruct, path, flag, out);
	return origSetWscScheme(schemestruct, path, flag, out);
}

std::pair<int, size_t> Scheme::getSchemeVersionAndSize() {
	size_t schemesize = 0;
	int retv;
	_asm lea eax, schemesize
	_asm push eax
	_asm mov esi, addrSchemeStruct
	_asm call addrGetSchemeVersion
	_asm mov retv, eax

//	debugf("Scheme version: %d size: %d\n", retv, schemesize);
	return {retv, schemesize};
}

int (__stdcall *origGetBuiltinSchemeName)();
int __stdcall Scheme::hookGetBuiltinSchemeName() {
	int a1, a2, retv;
	_asm mov a1, eax
	_asm mov a2, ecx

	_asm mov eax, a1
	_asm mov ecx, a2
	_asm call origGetBuiltinSchemeName
	_asm mov retv, eax

	if(a2 == 4)
		WaLibc::CStringFromString((void*)(a1+12), 0, (void*)missionDefaultSchemeName.c_str(), missionDefaultSchemeName.length());
	return retv;
}

char **Scheme::callGetBuiltinSchemeName(int id) {
	_asm mov eax, addrSchemeStruct
	_asm mov ecx, id
	_asm call hookGetBuiltinSchemeName

	return (char**)(addrSchemeStruct+12);
}


int __stdcall Scheme::hookSetBuiltinScheme(DWORD schemestruct, int id) {
	if(Missions::getFlagInjectingWam() || flagReadingWam) {
		return 0;
	}
	return origSetBuiltinScheme(schemestruct, id);
}

void Scheme::setMissionWscScheme(std::filesystem::path wam) {
	std::string wscpath, wscname;
	fs::path providedwsc = wam.parent_path() / (wam.stem().string() + ".wsc");
	bool out;
	if(fs::exists(providedwsc)) {
		hookSetWscScheme(addrSchemeStruct, (char*)providedwsc.string().c_str(), 0, &out);
		wscname = missionCustomSchemeName;
	} else {
		hookSetWscScheme(addrSchemeStruct, (char*)missionDefaultSchemePath.c_str(), 0, &out);
		wscname = missionDefaultSchemeName;
	}
	callReadWamSchemeSettings(wam.string());
	callReadWamSchemeOptions(wam.string());

	if(Packets::isHost()) {
		callRefreshOnlineMultiplayerSchemeDisplay();
		origSendWscScheme(LobbyChat::getLobbyHostScreen(), 0);
	} else if(!Packets::isClient()) {
		callRefreshOfflineMultiplayerSchemeDisplay();
	}

	setSchemeName(wscname);
}

DWORD Scheme::getAddrSchemeStruct() {
	return addrSchemeStruct;
}

void Scheme::callSetBuiltinScheme(int id) {
	origSetBuiltinScheme(addrSchemeStruct, id);
	static std::map<int, int> namemap = {
			{0, 970}, {2, 972}, {3, 971}, {4, 971}, {5, 981}, {6, 973}, {7, 974},
			{8, 980}, {9, 971}, {10, 978}, {11, 971}, {12, 979}, {13, 976}, {14, 977},
			{15, 975}, {16, 982}
	};
	if(namemap.find(id - 1) != namemap.end()) {
		setSchemeName(Frontend::origGetTextById(namemap.at(id - 1)));
	} else {
		setSchemeName(Frontend::origGetTextById(971));
	}
	if(Packets::isHost()) {
		callRefreshOnlineMultiplayerSchemeDisplay();
		origSendWscScheme(LobbyChat::getLobbyHostScreen(), 0);
	} else if(!Packets::isClient()) {
		callRefreshOfflineMultiplayerSchemeDisplay();
	}
}

void Scheme::loadSchemeFromBytestr(std::string data) {
	if(data.size() < 0x1E8) {
//		Utils::hexDump("Loaded 3.8 scheme", data.data(), data.size());
		memcpy((char *) (addrSchemeStruct + 0x14), data.data(), data.size());
		debugf("Restored scheme from replay json\n");
	} else {
		MessageBoxA(0, "Replay json file contains embedded WAM scheme, but it exceeds allowed scheme size.", Config::getFullStr().c_str(), MB_ICONERROR);
	}
}

std::optional<std::string> Scheme::saveSchemeToBytestr() {
	auto info = getSchemeVersionAndSize();
	int builtinscheme = *(int*)(Scheme::getAddrSchemeStruct() + 0x8);
	if(builtinscheme != 4) {
		// save custom scheme in json
		size_t size = 0;
		switch(info.first) {
			default:
			case 1: size = 0xDD - 5; break;
			case 2: size = 0x129 - 5; break;
			case 3: size = 0x129 + info.second - 5; break;
		}
		std::string scheme = std::string((char*)(Scheme::getAddrSchemeStruct() + 0x14), size);
		return {scheme};
	}
	return {};
}

void Scheme::setSchemeName(std::string name) {
	if(Config::isDontRenameSchemeComboBox()) return;
	DWORD comboScheme = 0;
	if(Packets::isHost()) {
		auto host = LobbyChat::getLobbyHostScreen();
		if(host)
			comboScheme = host + 0x425B0;
	} else if(!Packets::isClient()) {
		auto offline = LobbyChat::getLobbyOfflineScreen();
		if(offline)
			comboScheme = offline + 0x2B3A0;
	}
	if(comboScheme) {
		HWND comboSchemeHwnd = *(HWND*)((DWORD)comboScheme + 0x20);
		SetWindowTextA(comboSchemeHwnd, name.c_str());
	}
}

int (__stdcall *origReadWamSchemeSettings)();
int __stdcall Scheme::hookReadWamSchemeSettings() {
	int params, retv;
	_asm mov params, eax


	if(!Missions::getFlagInjectingWam()) {
		_asm mov eax, params
		_asm call origReadWamSchemeSettings
		_asm mov retv, eax
	}

	return retv;
}

int (__stdcall *origReadWamSchemeOptions)();
int __stdcall Scheme::hookReadWamSchemeOptions() {
	int params, retv;
	_asm mov params, eax

//	if(!Missions::getFlagInjectingWam()) {
		_asm mov eax, params
		_asm call origReadWamSchemeOptions
		_asm mov retv, eax
//	}

	return retv;
}

int Scheme::callReadWamSchemeSettings(std::string wampath) {
	Missions::WAMLoaderParams params;
	memset((void*)&params, 0, sizeof(params));
	params.path = (char*)wampath.c_str();

	flagReadingWam = true;
	int retv;
	_asm lea eax, params
	_asm call origReadWamSchemeSettings
	flagReadingWam = false;
	return retv;
}


int Scheme::callReadWamSchemeOptions(std::string wampath) {
	Missions::WAMLoaderParams params;
	memset((void*)&params, 0, sizeof(params));
	params.path = (char*)wampath.c_str();

	flagReadingWam = true;
	int retv;
	_asm lea eax, params
	_asm call origReadWamSchemeOptions
	_asm mov retv, eax
	flagReadingWam = false;
	return retv;
}

void Scheme::callRefreshOfflineMultiplayerSchemeDisplay() {
	DWORD screen = LobbyChat::getLobbyOfflineScreen();
	if(screen) {
		_asm mov eax, screen
		_asm call addrRefreshOfflineMultiplayerSchemeDisplay
	}
}

void Scheme::callRefreshOnlineMultiplayerSchemeDisplay() {
	DWORD screen = LobbyChat::getLobbyHostScreen();
	if(screen) {
		_asm mov eax, screen
		_asm call addrRefreshOnlineMultiplayerSchemeDisplay
	}
}


void Scheme::install() {
	DWORD addrGetSchemeSettingsFromWam = _ScanPattern("GetSchemeSettingsFromWam", "\x57\x6A\x04\x68\x00\x00\x00\x00\x8B\xF8\xE8\x00\x00\x00\x00\x83\xF8\xFF\x75\x04\x0B\xC0\x5F\xC3\x8B\x47\x0C\x56\x8B\x35\x00\x00\x00\x00\x50\x6A\x00\x68\x00\x00\x00\x00\x68\x00\x00\x00\x00\xFF\xD6", "????????xxx????xxxxxxxxxxxxxxx????xxxx????x????xx");
	DWORD addrGetBuiltinSchemeName = _ScanPattern("GetBuiltinSchemeName", "\x83\xC1\xFF\x83\xF9\x10\x0F\x87\x00\x00\x00\x00\xFF\x24\x8D\x00\x00\x00\x00\x83\xC0\x0C\x50\xBA\x00\x00\x00\x00\xE8\x00\x00\x00\x00\xB8\x00\x00\x00\x00\xC3", "??????xx????xxx????xxxxx????x????x????x");

	DWORD addrSetWscScheme = _ScanPattern("SetWscScheme", "\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x81\xEC\x00\x00\x00\x00\x53\x55\x8B\xAC\x24\x00\x00\x00\x00\x56\x8B\xB4\x24\x00\x00\x00\x00\x57\x8D\x4C\x24\x20", "???????xx????xxxx????xx????xxxxx????xxxx????xxxxx");
	addrGetSchemeVersion = _ScanPattern("GetSchemeVersion", "\xB8\x00\x00\x00\x00\xB9\x00\x00\x00\x00\x2B\xC8\x8D\x64\x24\x00\x8A\x94\x06\x00\x00\x00\x00\x3A\x14\x01\x75\x38\x83\xE8\x01\x75\xEF\x33\xC9\x8D\x86\x00\x00\x00\x00\x8D\xA4\x24\x00\x00\x00\x00", "??????????xxxxxxxxx????xxxxxxxxxxxxxx????xxx????");
	origSendWscScheme =
			(int (__stdcall *)(DWORD,char))
					_ScanPattern("SendWscScheme", "\x55\x8B\xEC\x83\xE4\xF8\x81\xEC\x00\x00\x00\x00\x56\x57\xBF\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x8B\x4D\x08\x89\x81\x00\x00\x00\x00\x8B\x15\x00\x00\x00\x00\xA0\x00\x00\x00\x00\x66\xC7\x44\x24\x00\x00\x00", "??????xx????xxx????x????xxxxx????xx????x????xxxx???");
	DWORD addrReadWamSchemeSettings = _ScanPattern("ReadWamSchemeSettings", "\x57\x6A\x04\x68\x00\x00\x00\x00\x8B\xF8\xE8\x00\x00\x00\x00\x83\xF8\xFF\x75\x04\x0B\xC0\x5F\xC3\x8B\x47\x0C\x56\x8B\x35\x00\x00\x00\x00\x50", "????????xxx????xxxxxxxxxxxxxxx????x");
	DWORD addrReadWamSchemeOptions = _ScanPattern("ReadWamSchemeOptions", "\x51\x53\x55\x56\x8B\x35\x00\x00\x00\x00\x57\x8B\xF8\x8B\x47\x0C\x50\x6A\x00\x68\x00\x00\x00\x00\x68\x00\x00\x00\x00\xFF\xD6\xA2\x00\x00\x00\x00", "??????????xxxxxxxxxx????x????xxx????");

	addrRefreshOfflineMultiplayerSchemeDisplay = _ScanPattern("RefreshOfflineMultiplayerSchemeDisplay", "\x51\x56\x57\x8B\xF8\xA0\x00\x00\x00\x00\x3C\x02\x75\x07\xB8\x00\x00\x00\x00\xEB\x0F\x33\xC9\x84\xC0\x0F\x95\xC1\x81\xC1\x00\x00\x00\x00\x8B\xC1\x8D\xB7\x00\x00\x00\x00", "??????????xxxxx????xxxxxxxxxxx????xxxx????");
	addrRefreshOnlineMultiplayerSchemeDisplay = _ScanPattern("RefreshOnlineMultiplayerSchemeDisplay", "\x51\x56\x57\x8B\xF8\x0F\xBE\x05\x00\x00\x00\x00\x89\x87\x00\x00\x00\x00\x0F\xBE\x0D\x00\x00\x00\x00\x89\x8F\x00\x00\x00\x00\x8B\xCF\xE8\x00\x00\x00\x00", "??????xx????xx????xxx????xx????xxx????");
	addrSchemeStruct = *(DWORD*)(addrGetSchemeSettingsFromWam + 0x4);
	addrSchemeStructAmmo = addrSchemeStruct + 0xEC;
	DWORD addrSetBuiltinScheme = (addrGetSchemeSettingsFromWam + 0xF + *(DWORD*)(addrGetSchemeSettingsFromWam + 0xB));

	debugf("addrSchemeStruct: 0x%X addrSetBuiltinScheme: 0x%X\n", addrSchemeStruct, addrSetBuiltinScheme);

	_HookDefault(SetWscScheme);
	_HookDefault(GetBuiltinSchemeName);
	_HookDefault(SetBuiltinScheme);
	_HookDefault(ReadWamSchemeSettings);
	_HookDefault(ReadWamSchemeOptions);

	dumpSchemeFromResources(286, missionDefaultSchemePath);
}



