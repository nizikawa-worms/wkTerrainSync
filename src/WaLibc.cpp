#include "WaLibc.h"
#include "Hooks.h"
#include "TerrainList.h"
#include "Config.h"

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
			sprintf_s(buff, "data\\level\\%s\\water.dir", terraininfo.dirname.c_str());
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

	DWORD addrWaFopenRef = Hooks::scanPattern("WaFopenRef", "\x81\xEC\x00\x00\x00\x00\x68\x00\x00\x00\x00\x50\xE8\x00\x00\x00\x00\x83\xC4\x08\x85\xC0\x89\x44\x24\x14\x0F\x84\x00\x00\x00\x00\x0F\xB7\x06", "??????x????xx????xxxxxxxxxxx????xxx");
	DWORD addrWaFopen = *(DWORD*)(addrWaFopenRef + 0xD) + addrWaFopenRef + 0x11;
	Hooks::hook("WaFopen", addrWaFopen, (DWORD *) &hookWaFopen, (DWORD *) &origWaFopen);
	return 0;
}