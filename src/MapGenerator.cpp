#include "MapGenerator.h"
#include "Hooks.h"
#include "Utils.h"
#include "Packets.h"
#include "WaLibc.h"
#include "BitmapImage.h"
#include <algorithm>
#include "FrontendDialogs.h"
#include "Config.h"
#include "LobbyChat.h"

int (__stdcall *origGenerateMapFromParams)(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12);
int __stdcall MapGenerator::hookGenerateMapFromParams(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12) {
	flagResizeBitmap = scaleXIncrements || scaleYIncrements;
	countResizeBitmap = 0;
	// a6 = seed
	// a8 = cavern
	// a9 = variant
	// a10 = decor
	// a11 = bridges
	if(flagResizeBitmap) {
		auto scale = std::max(getEffectiveScaleX(), getEffectiveScaleY());
		a10 *= scale;
		a11 *= scale;
//		printf("a1: 0x%X a2: 0x%X a3: 0x%X a4: 0x%X a5: 0x%X a6: 0x%X a7: 0x%X  a8: 0x%X a9: 0x%X a10: 0x%X a11: 0x%X a12: 0x%X\n", a1, a2, a3, a4, a5, a6, a7, a8, a9,  a10, a11, a12);
		if(flagExtraFeatures & extraBits::f_DecorHell) {
			a10 = 1000;
			a11 = 1000;
		}
	}
	auto ret = origGenerateMapFromParams(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);
	flagResizeBitmap = false;
	return ret;
}

void MapGenerator::onReseedAndWriteMapThumbnail(int a1) {
	int * mtype = (int*)(a1 + 0x554);
	if(scaleXIncrements || scaleYIncrements) {
		writeMagicMapTypeToThumbnail(mtype);
	}
}

void MapGenerator::writeMagicMapTypeToThumbnail(int *mtype) {
	if(mtype == 0) {
		if(mapThumbnailPtr) {
			mtype = (int*)(mapThumbnailPtr + 0x554);
		} else {
			return;
		}
	}
	if(*mtype != 2 && (*mtype & 0xFF) != magicMapType) {
		return;
	}

	FILE * f = fopen("data\\current.thm", "r+b");
	if(f) {
		if(scaleXIncrements || scaleYIncrements) {
			*mtype = encodeMagicMapType();
		} else {
			*mtype = 2;
		}
		fwrite(mtype, sizeof(int), 1, f);
		fclose(f);
	}
	if(Packets::isHost()) {
		Packets::resendMapDataToAllClients();
		Packets::resetPlayerBulbs();
	}
}

unsigned int MapGenerator::encodeMagicMapType() {
	return magicMapType | scaleXIncrements << 8 | scaleYIncrements << 16 | flagExtraFeatures << 24;
}

void MapGenerator::onMapTypeChecks(int a1) {
	mapTypeCheckThis = a1;
	DWORD * mtype = (DWORD*)(a1 + 0x554);
	if((*mtype & 0xFF) == magicMapType) {
		*mtype = 2;
	}
}

void MapGenerator::onConstructBitmapImage(int &width, int &height, int &bitdepth) {
//	printf("onConstructBitmapImage: %dx%d (%d,%d)\n", width, height,flagResizeBitmap, countResizeBitmap);
	if(flagResizeBitmap && countResizeBitmap <= 3) {
		if((width == 1920 && height == 696) || (width == 240 && height == 87) ) {
//			printf("onConstructBitmapImage scaling %d %d\n", width, height);
			width = getEffectiveWidth(width);
			height = getEffectiveHeight(height);
			countResizeBitmap++;
		}
	}
}


int (__stdcall *origSaveRandomMapAsIMG)(char *filename, int seed, int a3, int a4);
int __stdcall MapGenerator::hookSaveRandomMapAsIMG(char *filename, int seed, int cavern, int variant) {
//	printf("hookSaveRandomMapAsIMG: filename: %s seed: 0x%X cavern: 0x%X variant: 0x%X\n", filename, seed, cavern, variant);
	if(scaleXIncrements || scaleYIncrements) {
		flagResizeBitmap = true;
		countResizeBitmap = 0;
	}
	auto ret = origSaveRandomMapAsIMG(filename, seed, cavern, variant);
	flagResizeBitmap = false;
	countResizeBitmap = 0;
	return ret;
}

void MapGenerator::onTerrainPacket(DWORD This, DWORD EDX, unsigned char *packet, size_t size) {
//	Utils::hexDump("onTerrainPacket", packet, size);
	if(size >= 0x2C) {
		DWORD * mtype = (DWORD*)(packet + 0x8);
		auto prevScaleX = scaleXIncrements;
		auto prevScaleY = scaleYIncrements;
		if((*mtype & 0xFF) == magicMapType) {
			setScaleX((*mtype & 0xFF00) >> 8);
			setScaleY((*mtype & 0xFF0000) >> 16);
			setFlagExtraFeatures((*mtype & 0xFF000000) >> 24);
			*mtype = 2;
			Packets::resetPlayerBulbs();
		} else {
			resetScale();
		}
		if(Config::isPrintMapScaleInChat() && (prevScaleX != scaleXIncrements || prevScaleY != scaleYIncrements)) {
			printCurrentScale();
		}
	}
}

int (__stdcall *origConstructMapThumbnailCWnd)(DWORD a1, char a2);
int __stdcall MapGenerator::hookConstructMapThumbnailCWnd(DWORD a1, char a2) {
	mapThumbnailPtr = a1;
	return origConstructMapThumbnailCWnd(a1, a2);
}

int (__fastcall *origDestructMapThumbnailCWnd)(DWORD This);
int __fastcall MapGenerator::hookDestructMapThumbnailCWnd(DWORD This) {
	mapThumbnailPtr = 0;
	FrontendDialogs::destroyTerrainSizeComboBoxes();
	return origDestructMapThumbnailCWnd(This);
}

void MapGenerator::onFrontendExit() {
	resetScale(false);
}

void MapGenerator::onCreateReplay(nlohmann::json & config) {
	if(scaleXIncrements || scaleYIncrements) {
		config["scaleX"] = scaleXIncrements;
		config["scaleY"] = scaleYIncrements;
		config["extra"] = flagExtraFeatures;
	}
}

void MapGenerator::onLoadReplay(nlohmann::json & config) {
	if(config.contains("scaleX") && config.contains("scaleY")) {
		scaleXIncrements = config["scaleX"];
		scaleYIncrements = config["scaleY"];
		flagExtraFeatures = 0;
		if(config.contains("extra"))
			flagExtraFeatures = config["extra"];
	} else {
		resetScale(false);
	}
}

int MapGenerator::getScaleXIncrements() {
	return scaleXIncrements;
}

int MapGenerator::getScaleYIncrements() {
	return scaleYIncrements;
}

int (__stdcall *origMapPreviewScaling)();
int __stdcall MapGenerator::hookMapPreviewScaling() {
	int sesi, retv;
	_asm mov sesi, esi

	if(scaleXIncrements || scaleYIncrements) {
		DWORD * width = (DWORD *)(sesi + 3160);
		DWORD * height = (DWORD *)(sesi + 3164);
		DWORD * size = (DWORD *)(sesi + 3168);
		void ** data = (void **)(sesi + 3140);
		if(*width == 1920 && *height == 696 && *size == 0x146400) {
			*width = getEffectiveWidth();
			*height = getEffectiveHeight();
			*size = *width * *height;
			WaLibc::waFree(*data);
			*data = WaLibc::waMalloc(*size);
		}
	}

	_asm mov esi, sesi
	_asm call origMapPreviewScaling
	_asm mov retv, eax

	return retv;
}

void __stdcall MapGenerator::hookEditorGenerateMap_patch1_c(BitmapImage* src, BitmapImage * thumb) {
	auto putpixel = (void (__thiscall **)(BitmapImage *, int, int, int))(*(DWORD*)thumb + 20);
	auto getpixel = (int (__thiscall **)(BitmapImage*, int, int))(*(DWORD *)src + 16);
	for(int y=0; y < thumb->max_height_dword18; y++) {
		for(int x=0; x < thumb->max_width_dword14; x++) {
			int color = (*getpixel)(src, (int)(16 * x * getEffectiveScaleX()) & 0xFFFFF0, (int)(y * 16 * getEffectiveScaleY()) & 0xFFFFF0);
			(*putpixel)(thumb, x, y, color != 0);
		}
	}
}

DWORD addrEditorGenerateMap_patch1_ret;
void __declspec(naked) MapGenerator::hookEditorGenerateMap_patch1() {
	_asm push ebx
	_asm push ebp
	_asm call hookEditorGenerateMap_patch1_c
	_asm jmp addrEditorGenerateMap_patch1_ret
}


DWORD addrBakeMap_patch1_ret;
void __stdcall MapGenerator::hookBakeMap_patch1_c(DWORD ebp) {
	DWORD * width = (DWORD*)(ebp + 12280);
	DWORD * height = (DWORD*)(ebp + 12284);
	DWORD * size = (DWORD*)(ebp + 12288);
	void ** data = (void **)(ebp + 12260);

	*width = getEffectiveWidth();
	*height = getEffectiveHeight();

	void * copy = WaLibc::waMalloc(*width * *height);
	memcpy(copy, *data, *size);
	WaLibc::waFree(*data);

	*data = copy;
	*size = *width * *height;
}

void __declspec(naked) MapGenerator::hookBakeMap_patch1() {
	_asm push ebp
	_asm call hookBakeMap_patch1_c
//	_asm	mov     dword ptr [ebp+0x2FF8], 0x780
//	_asm	mov     dword ptr [ebp+0x2FFC], 0x2B8
//	_asm	mov     dword ptr [ebp+0x3000], 0x146400
	_asm jmp addrBakeMap_patch1_ret
}

int (__stdcall *origEditorGenerateMapVariant)(int a1, int cavern, int variant);
int __stdcall MapGenerator::hookEditorGenerateMapVariant(int a1, int cavern, int variant) {
	flagResizeBitmap = scaleXIncrements || scaleYIncrements;
	countResizeBitmap = 0;
	int ret = origEditorGenerateMapVariant(a1, cavern, variant);
	flagResizeBitmap = false;
	return ret;
}

int (__fastcall *origRandomStateHistoryWriter)(DWORD This, int EDX, int a2, int a3, int a4, int a5, int a6, int a7);
int __fastcall hookRandomStateHistoryWriter(DWORD This, int EDX, int a2, int a3, int a4, int a5, int a6, int a7) {
	// state history might overflow on big maps and overwrite FILE* with landgen.svg and crash on next fprintf
	int * step = (int*)(This + 0x8);
	unsigned int * seed = (unsigned int*)(This + 0xC);
	if(*step > 2044) {
		*seed = 0x19660D * *seed + 0x3C6EF35F;
		*step = *seed % 2044;
//		return 0;
	}
	return origRandomStateHistoryWriter(This, EDX, a2, a3, a4, a5, a6, a7);
}

int __stdcall MapGenerator::hookCreateBitThumbnail_patch1_c(int a1, int a2, int a3, int a4) {
	unsigned char * src = (unsigned  char*)(a2);
	unsigned char * dst = (unsigned  char*)(a3);

	short int width = *(short int*)(a2 - 4);
	short int height = *(short int*)(a2 - 2);
	if(width == 1920 && height == 696 || strncmp((char*)(a2 - 14), "IMG", 3)) return 0;

	int widthdiff = 0;
	if(mapThumbnailPtr && Config::isSuperFrontendThumbnailFix()) {
		///// super frontend map thumbnail size fix
		//	int *dstwidth = (int*)(a3 - 0x2C);  // not stable
		//	int *dstheight = (int*)(a3 - 0x28);
		HWND maphwnd = *(HWND*)((DWORD) mapThumbnailPtr + 0x20);
		RECT windowRect;
		GetWindowRect(maphwnd, &windowRect);
		int windowWidth = windowRect.right - windowRect.left;
		widthdiff = std::max(windowWidth - 240, 0);
	}

	float stepX = (float)getEffectiveWidth() / 240.0;
	float stepY = (float)getEffectiveHeight() / 87.0;
	float sizeX = getEffectiveWidth();
	float sizeY = getEffectiveHeight();

	// thumbnail
	int color = Config::getThumbnailColor();
	if(flagExtraFeatures & extraBits::f_DecorHell) {
		color = 116;
	}
	for(int ty=0; ty < 87; ty++) {
		for(int tx=0; tx < 240; tx++) {
			int mx = tx * stepX;
			int my = ty * stepY;
			float off = my * sizeX + mx;
			*dst = *(src + (int)off/8) != 0 ?  color : 0;
			dst++;
		}
		dst+=widthdiff;
	}

	// mono preview in mapgen
	stepX = (float)getEffectiveWidth() / 1920.0;
	stepY = (float)getEffectiveHeight() / 696.0;
	dst = (unsigned char*)a2;
	int acc=0;
//	memset(dst, 0, 0x28C80);
	for(int ty=0; ty < 696; ty++) {
		for(int tx=0; tx < 1920; tx+=8) {
			int mx = tx * stepX;
			int my = ty * stepY;
			float off = my * sizeX + mx;
			*dst = *(src + (int)off/8);
			dst++;
		}
	}

	if(getEffectiveScaleY() > 2.5) {
		// map editor checks if generated cavern map has top layer pixels at least X pixels thick, otherwise it rejects the map
		// tall maps are vertically scaled in the editor preview, so their top pixels get very thin in the preview, which might cause the map check to fail
		// to avoid this, with tall map previews top 32 pixels are filled with the color of the top-left pixel (0 for islands, FF for caverns)
		memset((unsigned  char*)(a2), *src, 240*32);
	}

	return 1;
}


DWORD addrCreateBitThumbnail_patch1_ret;
DWORD addrCreateBitThumbnail_patch1_ret_normal;
void __declspec(naked) MapGenerator::hookCreateBitThumbnail_patch1() {
	_asm push ebx
	_asm push esi
	_asm push ecx

	_asm push [ebp+0x14] // a4
	_asm push [ebp+0x10] // a3
	_asm push [ebp+0xC] // a2
	_asm push [ebp+0x8] // a1
	_asm call hookCreateBitThumbnail_patch1_c

	_asm pop ecx
	_asm pop esi
	_asm pop ebx

	_asm test eax,eax
	_asm jnz exitfunc
	_asm mov dword ptr ss:[esp+0xEC],esi
	_asm mov dword ptr ss:[esp+0xE0],0x57
	_asm jmp addrCreateBitThumbnail_patch1_ret_normal

	_asm exitfunc: jmp addrCreateBitThumbnail_patch1_ret
}

int __stdcall MapGenerator::hookW2PrvToEditor_patch1_c(int dst, int src, int width, int height) {
	if((!scaleXIncrements && !scaleYIncrements) || strncmp((char*)(src - 14), "IMG", 3)) return 0;

	float stepX = (float)getEffectiveWidth() / (float)width;
	float stepY = (float)getEffectiveHeight() / (float)height;
	float sizeX = getEffectiveWidth();
	float sizeY = getEffectiveHeight();

	unsigned char *dstptr = (unsigned char*)dst;
	unsigned char *srcptr = (unsigned char*)src;

	for(int ty=0; ty < height; ty++) {
		for(int tx=0; tx < width; tx++) {
			int mx = tx * stepX;
			int my = ty * stepY;
			float off = my * sizeX + mx;
			*dstptr = *(srcptr + (int)off/8);
			dstptr++;
		}
	}

	if(getEffectiveScaleY() > 2.5) {
		// see hookCreateBitThumbnail_patch1_c for details
		memset((unsigned char*)dst, *srcptr, width*32);
	}
	return 1;
}


DWORD addrW2PrvToEditor_patch1_ret;
DWORD addrW2PrvToEditor_patch1_ret_normal;
void __declspec(naked) MapGenerator::hookW2PrvToEditor_patch1() {
	_asm mov dword ptr ss:[esp+0xC],0

	_asm push esi // save esi just in case
	_asm push ebx // save ebx

	_asm push eax // height
	_asm push ecx // width
	_asm push ebx // src
	_asm push edi // dst
	_asm call hookW2PrvToEditor_patch1_c

	_asm test eax, eax
	_asm pop ebx // restore ebx
	_asm pop esi

	_asm jnz exitfunc2
	_asm mov     eax, [esp+0x24] // height
	_asm mov     ecx, [esp+0x20] // width
	_asm mov	 edi, [esp+0x2C] // dst
	_asm jmp addrW2PrvToEditor_patch1_ret_normal

	_asm exitfunc2:
	_asm mov     eax, [esp+0x24] // height
	_asm mov     ecx, [esp+0x20] // width
	_asm jmp addrW2PrvToEditor_patch1_ret
}

int (__stdcall *origExitMapEditor)(int a1);
int __stdcall MapGenerator::hookExitMapEditor(int a1) {
	int ret = origExitMapEditor(a1);
	writeMagicMapTypeToThumbnail(0);
	return ret;
}

void MapGenerator::install() {
	DWORD addrGenerateMapFromParams = Hooks::scanPattern("GenerateMapFromParams", "\x6A\xFF\x64\xA1\x00\x00\x00\x00\x68\x00\x00\x00\x00\x50\xB8\x00\x00\x00\x00\x64\x89\x25\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x8B\x84\x24\x00\x00\x00\x00\x53\x55\x33\xDB\x3B\xC3\x56\x57\x7D\x09\x89\x9C\x24\x00\x00\x00\x00\xEB\x10", "????????x????xx????xxx????x????xxx????xxxxxxxxxxxxx????xx");
	DWORD addrSaveRandomMapAsIMG = Hooks::scanPattern("SaveRandomMapAsIMG", "\x6A\xFF\x64\xA1\x00\x00\x00\x00\x68\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x81\xEC\x00\x00\x00\x00\x53\x8B\x9C\x24\x00\x00\x00\x00\x55\x33\xED\x3B\xDD\x56\x57\x7D\x04\x33\xDB\xEB\x0A\x83\xFB\x01\x7E\x05", "????????x????xxxx????xx????xxxx????xxxxxxxxxxxxxxxxxx");
	DWORD addrEditorGenerateMapVariant = Hooks::scanPattern("EditorGenerateMapVariant", "\x64\xA1\x00\x00\x00\x00\x6A\xFF\x68\x00\x00\x00\x00\x50\x8B\x44\x24\x14\x64\x89\x25\x00\x00\x00\x00\x81\xEC\x00\x00\x00\x00\x53\x55\x33\xED\x3B\xC5\x56\x57\x7D\x09\x89\xAC\x24\x00\x00\x00\x00\xEB\x10", "??????xxx????xxxxxxxx????xx????xxxxxxxxxxxxx????xx");
	DWORD addrConstructMapThumbnailCWnd = Hooks::scanPattern("ConstructMapThumbnailCWnd", "\x64\xA1\x00\x00\x00\x00\x6A\xFF\x68\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x53\x56\x8B\x74\x24\x18\x57\x56\xE8\x00\x00\x00\x00\x33\xDB\x89\x5C\x24\x14\xC7\x06\x00\x00\x00\x00\xC7\x46\x00\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x33\xC9", "??????xxx????xxxx????xxxxxxxxx????xxxxxxxx????xx?????x????xx");
	DWORD addrDestructMapThumbnailCWnd = Hooks::scanPattern("DestructMapThumbnailCWnd", "\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x51\x56\x8B\xF1\x89\x74\x24\x04\xC7\x06\x00\x00\x00\x00\xC7\x46\x00\x00\x00\x00\x00\xC7\x44\x24\x00\x00\x00\x00\x00\x8B\x86\x00\x00\x00\x00\x85\xC0\x74\x09\x50\xE8\x00\x00\x00\x00\x83\xC4\x04\x8D\x8E\x00\x00\x00\x00\xE8\x00\x00\x00\x00\xC6\x44\x24\x00\x00\x8B\x86\x00\x00\x00\x00\x83\xE8\x10\x8D\x48\x0C\x83\xCA\xFF\xF0\x0F\xC1\x11\x4A\x85\xD2\x7F\x0A\x8B\x08\x8B\x11\x50\x8B\x42\x04\xFF\xD0", "???????xx????xxxx????xxxxxxxxxx????xx?????xxx?????xx????xxxxxx????xxxxx????x????xxx??xx????xxxxxxxxxxxxxxxxxxxxxxxxxxxx");

	DWORD addrMapPreviewScaling = Hooks::scanPattern("MapPreviewScaling", "\x55\x8B\xEC\x83\xE4\xF8\x64\xA1\x00\x00\x00\x00\x6A\xFF\x68\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x83\xEC\x28\x83\xBE\x00\x00\x00\x00\x00\x57\x0F\x84\x00\x00\x00\x00\x6A\x00", "??????xx????xxx????xxxx????xxxxx?????xxx????xx");
	DWORD addrBakeMap = Hooks::scanPattern("BakeMap", "\x6A\xFF\x64\xA1\x00\x00\x00\x00\x68\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x83\xEC\x28\x53\x55\x8B\xE9\x33\xDB\x39\x9D\x00\x00\x00\x00\x56\x57\x0F\x85\x00\x00\x00\x00\x39\x9D\x00\x00\x00\x00\x8D\xB5\x00\x00\x00\x00\x74\x07\xE8\x00\x00\x00\x00\xEB\x06", "????????x????xxxx????xxxxxxxxxxx????xxxx????xx????xx????xxx????xx");

	DWORD addrRandomStateHistoryWriter = Hooks::scanPattern("RandomStateHistoryWriter", "\x83\xEC\x1C\x53\x55\x8B\x6C\x24\x2C\x56\x57\x8B\xD9\x8D\x49\x00\x8B\x7C\x24\x3C\x8B\x74\x24\x38\x2B\x74\x24\x30\x2B\xFD\x57\x56\x89\x74\x24\x2C\x89\x7C\x24\x30\xE8\x00\x00\x00\x00\x85\xC0", "??????xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx????xx");
	DWORD addrCreateBitThumbnail = Hooks::scanPattern("CreateBitThumbnail", "\x55\x8B\xEC\x83\xE4\xF8\x81\xEC\x00\x00\x00\x00\x53\x56\x57\xB0\xA0\x88\x84\x24\x00\x00\x00\x00\x88\x84\x24\x00\x00\x00\x00\xB0\xA1\x88\x84\x24\x00\x00\x00\x00\x88\x84\x24\x00\x00\x00\x00\xB0\xA3", "??????xx????xxxxxxxx????xxx????xxxxx????xxx????xx");
	DWORD addrW2PrvToEditor = Hooks::scanPattern("W2PrvToEditor", "\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x24\x53\x8B\x5D\x0C\x56\x8B\xF1\x8B\x8E\x00\x00\x00\x00\x57\x8B\xF8\x8B\x45\x08\x51\x89\x86\x00\x00\x00\x00\x89\x9E\x00\x00\x00\x00\x89\xBE\x00\x00\x00\x00", "??????xxxxxxxxxxxx????xxxxxxxxx????xx????xx????");

	DWORD addrExitMapEditor = Hooks::scanPattern("ExitMapEditor", "\x55\x8B\xEC\x83\xE4\xF8\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x51\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x53\x8B\x5D\x08\x8B\x83\x00\x00\x00\x00\x56\x57\x50\xE8\x00\x00\x00\x00\x8B\x43\x20\x83\xC4\x04", "??????xxx????xx????xxxx????xx????x????xxxxxx????xxxx????xxxxxx");

	Hooks::hook("GenerateMapFromParams", addrGenerateMapFromParams, (DWORD *) &hookGenerateMapFromParams, (DWORD *) &origGenerateMapFromParams);
	Hooks::hook("SaveRandomMapAsIMG", addrSaveRandomMapAsIMG, (DWORD*) &hookSaveRandomMapAsIMG, (DWORD*)&origSaveRandomMapAsIMG);
	Hooks::hook("EditorGenerateMapVariant", addrEditorGenerateMapVariant, (DWORD*) &hookEditorGenerateMapVariant, (DWORD*)&origEditorGenerateMapVariant);
	Hooks::hook("ConstructMapThumbnailCWnd", addrConstructMapThumbnailCWnd, (DWORD *) &hookConstructMapThumbnailCWnd, (DWORD *) &origConstructMapThumbnailCWnd);
	Hooks::hook("DestructMapThumbnailCWnd", addrDestructMapThumbnailCWnd, (DWORD *) &hookDestructMapThumbnailCWnd, (DWORD *) &origDestructMapThumbnailCWnd);
	Hooks::hook("MapPreviewScaling", addrMapPreviewScaling, (DWORD *) &hookMapPreviewScaling, (DWORD *) &origMapPreviewScaling);
	Hooks::hook("RandomStateHistoryWriter", addrRandomStateHistoryWriter, (DWORD *) &hookRandomStateHistoryWriter, (DWORD *) &origRandomStateHistoryWriter);
	Hooks::hook("ExitMapEditor", addrExitMapEditor, (DWORD *) &hookExitMapEditor, (DWORD *) &origExitMapEditor);

//	 fix generating 8 thumb%d.dat thumbnails in map editor
	DWORD addrEditorGenerateMap_patch1 = addrEditorGenerateMapVariant + 0x1A2; //579BE2
	addrEditorGenerateMap_patch1_ret = addrEditorGenerateMapVariant + 0x1F6; //579C36
	printf("addrEditorGenerateMap_patch1: 0x%X ret: 0x%X\n", addrEditorGenerateMap_patch1, addrEditorGenerateMap_patch1_ret);
	Hooks::hookAsm(addrEditorGenerateMap_patch1, (DWORD)&hookEditorGenerateMap_patch1);

	// fix baking preview as PNG
	DWORD addrBakeMap_patch1 = addrBakeMap + 0x17A; //48A2FA
	addrBakeMap_patch1_ret = addrBakeMap + 0x198; //048A318
	printf("addrBakeMap_patch1: 0x%X ret: 0x%X\n", addrBakeMap_patch1, addrBakeMap_patch1_ret);
	Hooks::hookAsm(addrBakeMap_patch1, (DWORD)&hookBakeMap_patch1);

	DWORD addrCreateBitThumbnail_patch1 = addrCreateBitThumbnail + 0x48B; //
	addrCreateBitThumbnail_patch1_ret = addrCreateBitThumbnail + 0x6A8; //
	addrCreateBitThumbnail_patch1_ret_normal = addrCreateBitThumbnail + 0x49D; //
	printf("addrCreateBitThumbnail_patch1: 0x%X ret: 0x%X\n", addrCreateBitThumbnail_patch1, addrCreateBitThumbnail_patch1_ret);
	Hooks::hookAsm(addrCreateBitThumbnail_patch1, (DWORD)&hookCreateBitThumbnail_patch1);
//
	DWORD addrW2PrvToEditor_patch1 = addrW2PrvToEditor + 0x103; //43E293
	addrW2PrvToEditor_patch1_ret_normal = addrW2PrvToEditor + 0x10D;
	addrW2PrvToEditor_patch1_ret = addrW2PrvToEditor + 0x136;
	printf("addrW2PrvToEditor_patch1: 0x%X ret: 0x%X\n", addrW2PrvToEditor_patch1, addrW2PrvToEditor_patch1_ret);
	Hooks::hookAsm(addrW2PrvToEditor_patch1, (DWORD)&hookW2PrvToEditor_patch1);
}

float MapGenerator::getEffectiveScaleX() {
	return std::min(1.0f + (float)scaleXIncrements * scaleIncrement, scaleMax);
}

float MapGenerator::getEffectiveScaleY() {
	return std::min(1.0f + (float)scaleYIncrements * scaleIncrement, scaleMax);
}

int MapGenerator::getEffectiveWidth(int width) {
	return (int)(width * getEffectiveScaleX()) & 0xFFFFFFF8;
}
int MapGenerator::getEffectiveHeight(int height) {
	return (int)(height * getEffectiveScaleY()) & 0xFFFFFFF8;
}

void MapGenerator::setScaleX(int scaleX, bool resetCombo) {
	if(scaleX < 0) scaleX = 0;
	scaleX = std::max(0, scaleX);
	scaleX = std::min((int)(scaleMax / scaleIncrement), scaleX);
	MapGenerator::scaleXIncrements = scaleX;
	printf("MapGenerator::SetScaleX: %f\n", getEffectiveScaleX());
	writeMagicMapTypeToThumbnail(0);
	if(resetCombo) {
		FrontendDialogs::resetComboBoxesText();
	}
}

void MapGenerator::setScaleY(int scaleY, bool resetCombo) {
	scaleY = std::max(0, scaleY);
	scaleY = std::min((int)(scaleMax / scaleIncrement), scaleY);
	MapGenerator::scaleYIncrements = scaleY;
	printf("MapGenerator::SetScaleY: %f\n", getEffectiveScaleY());
	writeMagicMapTypeToThumbnail(0);
	if(resetCombo) {
		FrontendDialogs::resetComboBoxesText();
	}
}

void MapGenerator::resetScale(bool writethumb) {
	if(scaleXIncrements || scaleYIncrements) {
		printf("MapGenerator::resetScale\n");
	}
	scaleXIncrements = scaleYIncrements = flagExtraFeatures = 0;
	FrontendDialogs::resetComboBoxesText();
	if(writethumb) {
		writeMagicMapTypeToThumbnail(0);
	}
}

unsigned int MapGenerator::getFlagExtraFeatures() {
	return flagExtraFeatures;
}

void MapGenerator::setFlagExtraFeatures(unsigned int flagExtraFeatures) {
	MapGenerator::flagExtraFeatures = flagExtraFeatures;
}

void MapGenerator::printCurrentScale() {
	char buff[256];
	sprintf_s(buff, "Map scale: %.01f x %.01fx", MapGenerator::getEffectiveScaleX(), MapGenerator::getEffectiveScaleY());
	LobbyChat::lobbyPrint(buff);
}
