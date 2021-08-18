#include "BitmapImage.h"
#include "Hooks.h"
#include "W2App.h"
#include "MapGenerator.h"

DWORD addrLoadImgFromDir;
BitmapImage * BitmapImage::callLoadImgFromDir(char * filename, DWORD a2, DWORD a3) {
	BitmapImage * bitmap;

	_asm mov eax, filename
	_asm mov ecx, a2
	_asm push a3
	_asm call addrLoadImgFromDir
	_asm mov bitmap, eax

	return bitmap;
}

DWORD addrLoadImgFromFile;
BitmapImage* BitmapImage::callLoadImgFromFile(DWORD DD_Display, DWORD Palette, char * path) {
	if(!DD_Display)
		DD_Display = W2App::getAddrDdDisplay();
	if(!Palette)
		Palette = *(DWORD*)(W2App::getAddrDdDisplay() + 0x3120); // img palette
	BitmapImage* retv;
	_asm mov esi, DD_Display
	_asm push path
	_asm push Palette
	_asm call addrLoadImgFromFile
	_asm mov retv, eax
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

	return retv;
}


void BitmapImage::install() {
	addrLoadImgFromDir = _ScanPattern("LoadImgFromDir", "\x53\x8B\x5C\x24\x08\x56\x57\x8B\xF1\x56\x8B\xF8\xE8\x00\x00\x00\x00\x85\xC0\x74\x11\x8B\x16\x8B\x40\x04\x8B\x52\x08\x50\x8B\xCE\xFF\xD2\x85\xC0\x75\x2F", "??????xxxxxxx????xxxxxxxxxxxxxxxxxxxxx");
	addrLoadImgFromFile = _ScanPattern("LoadImgFromFile", "\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x83\xEC\x0C\x8B\x44\x24\x20\x56\x68\x00\x00\x00\x00\x50\xC7\x44\x24\x00\x00\x00\x00\x00", "???????xx????xxxx????xxxxxxxxx????xxxx?????");
	DWORD addrConstructBitmapImage = _ScanPattern("ConstructBitmapImage", "\x53\x55\x8B\x6C\x24\x0C\x8B\xC5\x0F\xAF\xC1\x83\xC0\x07\x99\x83\xE2\x07\x03\xC2\xC1\xF8\x03\x83\xC0\x03\x83\xE0\xFC\x89\x46\x10\x0F\xAF\xC7", "??????xxxxxxxxxxxxxxxxxxxxxxxxxxxxx"); //0x4F6370

	_HookDefault(ConstructBitmapImage);
}
