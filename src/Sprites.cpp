
#include "Sprites.h"
#include "Hooks.h"
#include <set>
#include "Config.h"
#include "W2App.h"
#include "WaLibc.h"
#include "TerrainList.h"
#include "MapGenerator.h"
#include "Debugf.h"


DWORD __fastcall Sprites::hookLoadSpriteFromTerrain(DWORD DDDisplay, DWORD edx, int palette_num_a2, int sprite_id_a3, int vfs_a4, const char *filename) {
	if(Config::isDebugSpriteImg()) debugf("Loading sprite: index: %d filename: %s\n", sprite_id_a3, filename);
	DWORD ret = 0;
	for(auto & cb : onLoadSpriteFromTerrainCallbacks) {
		if(cb(DDDisplay, palette_num_a2, sprite_id_a3, vfs_a4, filename)) ret = 1;
	}
	if(ret) return 0;

	if(!strcmp(filename, "back.spr")) {
		auto & backSprExceptions = Config::getBackSprExceptions();
		auto & lastTerrainInfo = TerrainList::getLastTerrainInfo();
		if(callCheckIfFileExistsInVFS("_back.spr", vfs_a4)) {
			// custom terrains using extended loader for back.spr should rename the file as "_back.spr".
			hookLoadSpriteFromVFS(DDDisplay, 0, palette_num_a2, sprite_id_a3, 0, vfs_a4, "_back.spr");
		} else if(lastTerrainInfo && backSprExceptions.find(lastTerrainInfo->hash) != backSprExceptions.end()) {
			// existing custom terrains with custom back.spr are handled by a list of exceptions to maintain compatibility
			hookLoadSpriteFromVFS(DDDisplay, 0, palette_num_a2, sprite_id_a3, 0, vfs_a4, filename);
		} else {
			origLoadSpriteFromTerrain(DDDisplay, 0, palette_num_a2, sprite_id_a3, vfs_a4, filename);
		}
		if(Config::isAdditionalParallaxLayersAllowed()) {
			hookLoadSpriteFromVFS(DDDisplay, 0, palette_num_a2, back2SprId, 0, vfs_a4, "back2.spr");
			hookLoadSpriteFromVFS(DDDisplay, 0, palette_num_a2, frontSprId, 0, vfs_a4, "front.spr");
		}
		ret = 1;
	} else {
		ret = origLoadSpriteFromTerrain(DDDisplay, edx, palette_num_a2, sprite_id_a3, vfs_a4, filename);
	}
	if(Config::isDebugSpriteImg()) debugf("\t%s loading result: %d\n", filename, ret);
	return ret;
}
DWORD overrideSprite(DWORD DD_Display, int palette, int index, int vfs, const char *filename) {
	char buff[256];
	DWORD res;
	auto w2wrapper = W2App::getAddrW2Wrapper();
	bool colorblind = (w2wrapper && *(DWORD*)(w2wrapper + 0xF374));
	if(!colorblind)
		_snprintf_s(buff, _TRUNCATE, "gfx0\\%s|%s", filename, filename);
	else
		_snprintf_s(buff, _TRUNCATE, "gfx1\\%s|%s", filename, filename);

	res = (DWORD)Sprites::origLoadSpriteFromVFS(DD_Display, 0, palette, index, 0, vfs, buff);
	if(res) {
		debugf("Replaced sprite %s (id: %d) with version from terrain dir (%s)\n", filename, index, buff);
	}
	return res;
}

BitmapImage * overrideImg(DWORD DD_Display, DWORD vfs, const char * filename, int palette_id) {
	char buff[256];
	auto w2wrapper = W2App::getAddrW2Wrapper();
	bool colorblind = (w2wrapper && *(DWORD*)(w2wrapper + 0xF374));
	if(!colorblind)
		_snprintf_s(buff, _TRUNCATE, "gfx0\\%s|%s", filename, filename);
	else
		_snprintf_s(buff, _TRUNCATE, "gfx1\\%s|%s", filename, filename);

	DWORD palette = *(DWORD*)(DD_Display + 4 * palette_id + 0x311C);
	BitmapImage * img = BitmapImage::callLoadImgFromDir(buff, vfs, palette);
	return img;
}

DWORD __fastcall Sprites::hookLoadSpriteFromVFS(DWORD DD_Display, int EDX, int palette, int index, int a4, int vfs_a5, const char *filename) {
	if(Config::isDebugSpriteImg()) debugf("Loading sprite: index: %d filename: %s\n", index, filename);
	DWORD ret = 0;
	for(auto & cb : onLoadSpriteFromVFSCallbacks) {
		if(cb(DD_Display, palette, index, a4, vfs_a5, filename)) ret = 1;
	}
	if(ret) return 0;

	auto ddgame = W2App::getAddrDdGame();
	if(ddgame && Config::isSpriteOverrideAllowed()) {
		DWORD pclandscape = *(DWORD*)(ddgame + 0x4CC);
		if(pclandscape) {
			DWORD terrainvfs = *(DWORD*)(pclandscape + 0xB34);
			if(terrainvfs) {
				if(strcmp(filename, "debris.spr")) {
					ret = overrideSprite(DD_Display, palette, index, terrainvfs, filename);
				}
			}
		}
	}
	if(!ret) {
		ret = origLoadSpriteFromVFS(DD_Display, EDX, palette, index, a4, vfs_a5, filename);
	}
	if(Config::isDebugSpriteImg()) debugf("\t%s loading result: %d\n", filename, ret);
	return ret;
}

int (__fastcall *origDrawBackSprite)(int *a1, int a2, int a3, int a4, int a5, int a6, int sprite_a7, int a8);
int __fastcall Sprites::hookDrawBackSprite(int *a1, int a2, int a3, int a4, int a5, int a6, int sprite_a7, int a8) {
	auto ret = origDrawBackSprite(a1, a2, a3, a4, a5, a6, sprite_a7, a8);
	static bool allowparallax = Config::isAdditionalParallaxLayersAllowed();
	if(allowparallax) {
		static int la4 = 0;
		static int la8 = 0;
		if (sprite_a7 == 0x1026C) {
			la4 = a4;
			la8 = a8;
		}
		if (sprite_a7 == 0x1026D) {
			static int hide = Config::getParallaxHideOnBigMaps(); // todo: scale parallax params
			if (MapGenerator::getScaleYIncrements() == 0 || !hide) {
				static int backA = Config::getParallaxBackA();
				static int backB = Config::getParallaxBackB();
//			origDrawBackSprite(a1, a2*2.75, a3 + 1, la4, a5*1.125 , 3, 0x10000 | back2SprId, la8);
				origDrawBackSprite(a1, backA, a3 + 1, la4, backB, 3, 0x10000 | back2SprId, la8);
				static int frontA = Config::getParallaxFrontA();
				static int frontB = Config::getParallaxFrontB();
				origDrawBackSprite(a1, frontA, 0x1A000, 0, frontB, 1, 0x10000 | frontSprId, la8);
			}
		}
	}
	return ret;
}

DWORD addrCheckIfFileExistsInVFS;
int Sprites::callCheckIfFileExistsInVFS(const char * filename, DWORD vfs) {
	int reta;

	_asm push vfs
	_asm mov eax, filename
	_asm call addrCheckIfFileExistsInVFS
	_asm mov reta, eax

	return reta;
}

int Sprites::install() {
	DWORD addrConstructSprite = _ScanPattern("ConstructSprite", "\x89\x48\x04\x33\xC9\xC7\x00\x00\x00\x00\x00\xC7\x40\x00\x00\x00\x00\x00\x89\x48\x48\x89\x48\x4C\x89\x48\x44\x89\x48\x3C\x89\x48\x50\x89\x48\x54\x89\x48\x58\x89\x48\x5C\xC7\x40\x00\x00\x00\x00\x00", "??????x????xx?????xxxxxxxxxxxxxxxxxxxxxxxxxx?????");
	DWORD *addrSpriteVTable = *(DWORD**)(addrConstructSprite + 0x7);
	DWORD addrDestroySprite = addrSpriteVTable[0];
	DWORD addrDrawBackSprite = _ScanPattern("DrawBackSprite", "\x8B\x81\x00\x00\x00\x00\x3D\x00\x00\x00\x00\x56\x8B\x74\x24\x10\x57\x8B\x7C\x24\x10\x7D\x74\x53\x8B\x19\x83\xC3\xDC\x78\x6B\x89\x19\x8D\x5C\x0B\x04\x89\x9C\x81\x00\x00\x00\x00\x8B\x99\x00\x00\x00\x00\x8B\x84\x99\x00\x00\x00\x00\x83\xC3\x01\x85\xC0", "??????x????xxxxxxxxxxxxxxxxxxxxxxxxxxxxx????xx????xxx????xxxxx");
	DWORD addrLoadSpriteFromTerrain = _ScanPattern("LoadSpriteFromTerrain", "\x53\x8B\x5C\x24\x0C\xF7\xC3\x00\x00\x00\x00\x56\x8B\xF1\x74\x0A\x5E\xB8\x00\x00\x00\x00\x5B\xC2\x10\x00\x8B\x4C\x24\x0C\x8B\x06\x8B\x50\x14\x51", "??????x????xxxxxxx????xxxxxxxxxxxxxx");
	DWORD addrLoadSpriteFromVFS = _ScanPattern("LoadSpriteFromVFS", "\x55\x8B\x6C\x24\x0C\xF7\xC5\x00\x00\x00\x00\x56\x8B\xF1\x74\x0A\x5E\xB8\x00\x00\x00\x00\x5D\xC2\x14\x00\x53\x8B\x5C\x24\x10\x8D\x43\xFF\x83\xF8\x02\x0F\x87\x00\x00\x00\x00\x83\xBC\x9E\x00\x00\x00\x00\x00\x0F\x84\x00\x00\x00\x00", "??????x????xxxxxxx????xxxxxxxxxxxxxxxxx????xxx?????xx????");
	addrCheckIfFileExistsInVFS = _ScanPattern("CheckIfFileExistsInVFS", "\x53\x55\x8B\x6C\x24\x0C\x56\x57\x8B\xF0\x8D\x9B\x00\x00\x00\x00\x56\x68\x00\x00\x00\x00\x68\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x6A\x7C\x68\x00\x00\x00\x00\xE8\x00\x00\x00\x00", "??????xxxxxx????xx????x????x????xxx????x????");

	_HookDefault(LoadSpriteFromTerrain);
	_HookDefault(DrawBackSprite);
	_HookDefault(LoadSpriteFromVFS);
	return 0;
}

void Sprites::registerOnLoadSpriteFromVFSCallback(int(__stdcall * callback)(DWORD, int, int, int, int, const char*)) {
	onLoadSpriteFromVFSCallbacks.push_back(callback);
}
void Sprites::registerOnLoadSpriteFromTerrainCallback(int(__stdcall * callback)(DWORD, int, int, int,  const char*)) {
	onLoadSpriteFromTerrainCallbacks.push_back(callback);
}