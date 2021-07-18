#include "WaLibc.h"
#include "Hooks.h"
#include "TerrainList.h"
#include "Config.h"
#include <filesystem>

void *(__cdecl * origWaNew)(size_t size);
void *WaLibc::waMalloc(size_t size) {
	return origWaNew(size);
}

void (__cdecl *origWaFree)(void * ptr);
void WaLibc::waFree(void *ptr) {
	origWaFree(ptr);
}

FILE *(__cdecl *origWaFopen)(char *Filename, char *Mode);
FILE *__cdecl hookWaFopen(char *Filename, char *Mode) {
	if(!strcmp(Filename, "data\\Gfx\\Water.dir")) {
		char buff[MAX_PATH];
		auto terraininfo = TerrainList::getLastTerrainInfo();
		if(!terraininfo.dirname.empty() && terraininfo.hasWaterDir && Config::isCustomWaterAllowed()) {
			sprintf_s(buff, "%s\\data\\level\\%s\\water.dir", Config::getWaDir().string().c_str(), terraininfo.dirname.c_str());
			FILE * pf = fopen(buff, "rb");
			if(pf) {
				fclose(pf);
				printf("hookWaFopen: using %s instead of %s\n", buff, Filename);
				return origWaFopen(buff, Mode);
			}
		}
	}
	return origWaFopen(Filename, Mode);
}

int WaLibc::install() {
	DWORD addrWaMallocMemset = Hooks::scanPattern("WaMallocMemset", "\x8D\x47\x03\x83\xE0\xFC\x83\xC0\x20\x56\x50\xE8\x00\x00\x00\x00\x57\x8B\xF0\x6A\x00\x56\xE8\x00\x00\x00\x00\x83\xC4\x10\x8B\xC6\x5E\xC3", "??????xxxxxx????xxxxxxx????xxxxxxx");
	origWaNew = (void *(__cdecl *)(size_t)) (addrWaMallocMemset + 0x10 +  *(DWORD*)(addrWaMallocMemset +  0xC)); //5C0377
	origWaFree = (void (__cdecl *)(void *))Hooks::scanPattern("WaFree", "\x6A\x0C\x68\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x8B\x75\x08\x85\xF6\x74\x75\x83\x3D\x00\x00\x00\x00\x00\x75\x43\x6A\x04\xE8\x00\x00\x00\x00\x59\x83\x65\xFC\x00", "???????x????xxxxxxxxx?????xxxxx????xxxxx");
	DWORD addrGetDataPath = Hooks::scanPattern("GetDataPath", "\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x83\xEC\x08\x53\x56\x8B\x74\x24\x20\x33\xDB\x89\x5C\x24\x18\x89\x5C\x24\x0C\xE8\x00\x00\x00\x00\x33\xC9", "???????xx????xxxx????xxxxxxxxxxxxxxxxxxxx????xx");
	addrDriveLetter = *(char**)(addrGetDataPath + 0x69);
	addrSteamFlag = *(char**)(addrGetDataPath + 0x56);
	printf("addrDriveLetter: 0x%X addrSteamFlag: 0x%X\n", addrDriveLetter, addrSteamFlag);

	DWORD addrWaFopenRef = Hooks::scanPattern("WaFopenRef", "\x81\xEC\x00\x00\x00\x00\x68\x00\x00\x00\x00\x50\xE8\x00\x00\x00\x00\x83\xC4\x08\x85\xC0\x89\x44\x24\x14\x0F\x84\x00\x00\x00\x00\x0F\xB7\x06", "??????x????xx????xxxxxxxxxxx????xxx");
	DWORD addrWaFopen = *(DWORD*)(addrWaFopenRef + 0xD) + addrWaFopenRef + 0x11;

	CStringFromString =
			(int (__fastcall *)(void *,int,void *,size_t))
			Hooks::scanPattern("CStringFromString", "\x53\x56\x8B\x74\x24\x10\x85\xF6\x8B\xD9\x75\x0A\xE8\x00\x00\x00\x00\x5E\x5B\xC2\x08\x00\x8B\x4C\x24\x0C\x85\xC9", "??????xxxxxxx????xxxxxxxxxxx");

	Hooks::hook("WaFopen", addrWaFopen, (DWORD *) &hookWaFopen, (DWORD *) &origWaFopen);
	return 0;
}

std::string WaLibc::getWaDataPath(bool addWaPath) {
	if(!addrDriveLetter || !addrSteamFlag) return "";
	if(*addrSteamFlag) {
		if(!addWaPath) return "Data";
		else return (Config::getWaDir() / "Data").string();
	}
	if(*addrDriveLetter) {
		char buff[16];
		sprintf_s(buff, "%c:\\Data", *addrDriveLetter);

		std::filesystem::path path(buff);
		if(!std::filesystem::exists(path)) return "";

		return buff;
	} else {
		return "";
	}
}
