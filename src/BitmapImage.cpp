#include "BitmapImage.h"
#include "Hooks.h"
#include "W2App.h"
#include "MapGenerator.h"
#include "Config.h"
#include "Debugf.h"

DWORD addrLoadImgFromDir;
BitmapImage * BitmapImage::callLoadImgFromDir(char * filename, DWORD a2, DWORD a3) {
	BitmapImage * bitmap;
	if(Config::isDebugSpriteImg()) debugf("Loading IMG: %s\n", filename);
	_asm mov eax, filename
	_asm mov ecx, a2
	_asm push a3
	_asm call addrLoadImgFromDir
	_asm mov bitmap, eax
	if(Config::isDebugSpriteImg()) debugf("\t%s loaded at: 0x%X\n", filename, bitmap);
	return bitmap;
}

DWORD addrLoadImgFromFile;
BitmapImage* BitmapImage::callLoadImgFromFile(DWORD DD_Display, DWORD Palette, char * path) {
	if(!DD_Display)
		DD_Display = W2App::getAddrDdDisplay();
	if(!Palette)
		Palette = *(DWORD*)(W2App::getAddrDdDisplay() + 0x3120); // img palette
	BitmapImage* retv;
	if(Config::isDebugSpriteImg()) debugf("Loading IMG: %s\n", path);
	_asm mov esi, DD_Display
	_asm push path
	_asm push Palette
	_asm call addrLoadImgFromFile
	_asm mov retv, eax
	if(Config::isDebugSpriteImg()) debugf("\t%s loaded at: 0x%X\n", path, retv);
	return retv;
}

DWORD origConstructBitmapImage;
BitmapImage * __stdcall BitmapImage::hookConstructBitmapImage(int width) {
	int bitdepth, height;
	BitmapImage * This, * retv;
	_asm mov bitdepth, ecx
	_asm mov height, edi
	_asm mov This, esi

	MapGenerator::onConstructBitmapImage(width, height, bitdepth);

	_asm push width
	_asm mov ecx, bitdepth
	_asm mov edi, height
	_asm mov esi, This
	_asm call origConstructBitmapImage
	_asm mov retv, eax

//	if(Config::isDebugSpriteImg()) debugf("Constructing bitmap: %dx%d@%d = 0x%X\n", width, height, bitdepth, retv);

	return retv;
}

//optional hook
DWORD origLoadImgFromDir;
BitmapImage* __stdcall BitmapImage::hookLoadImgFromDir(int a3) {
	// const char *a1@<eax>, int a2@<ecx>,
	char * filename;
	int a2;
	BitmapImage * bitmap;
	_asm mov filename, eax
	_asm mov a2, ecx

	debugf("Loading IMG: %s\n", filename);

	_asm mov eax, filename
	_asm mov ecx, a2
	_asm push a3
	_asm call origLoadImgFromDir
	_asm mov bitmap, eax

	debugf("\t%s loaded at: 0x%X\n", filename, bitmap);

	return bitmap;
}

//optional hook
DWORD origLoadImgFromFile;
BitmapImage * __stdcall BitmapImage::hookLoadImgFromFile(int a2, char *filename) {
	int a1;
	BitmapImage * bitmap;
	_asm mov a1, esi

	debugf("Loading IMG: %s\n", filename);

	_asm push filename
	_asm push a2
	_asm mov esi, a1
	_asm call origLoadImgFromFile
	_asm mov bitmap, eax

	debugf("\t%s loaded at: 0x%X\n", filename, bitmap);

	return bitmap;
}

void BitmapImage::install() {
	addrLoadImgFromDir = _ScanPattern("LoadImgFromDir", "\x53\x8B\x5C\x24\x08\x56\x57\x8B\xF1\x56\x8B\xF8\xE8\x00\x00\x00\x00\x85\xC0\x74\x11\x8B\x16\x8B\x40\x04\x8B\x52\x08\x50\x8B\xCE\xFF\xD2\x85\xC0\x75\x2F", "??????xxxxxxx????xxxxxxxxxxxxxxxxxxxxx");
	addrLoadImgFromFile = _ScanPattern("LoadImgFromFile", "\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x83\xEC\x0C\x8B\x44\x24\x20\x56\x68\x00\x00\x00\x00\x50\xC7\x44\x24\x00\x00\x00\x00\x00", "???????xx????xxxx????xxxxxxxxx????xxxx?????");
	DWORD addrConstructBitmapImage = _ScanPattern("ConstructBitmapImage", "\x53\x55\x8B\x6C\x24\x0C\x8B\xC5\x0F\xAF\xC1\x83\xC0\x07\x99\x83\xE2\x07\x03\xC2\xC1\xF8\x03\x83\xC0\x03\x83\xE0\xFC\x89\x46\x10\x0F\xAF\xC7", "??????xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"); //0x4F6370

	_HookDefault(ConstructBitmapImage);

	if(Config::isDebugSpriteImg()) {
		_HookDefault(LoadImgFromDir);
		_HookDefault(LoadImgFromFile);
	}

}
