#include "BitmapImage.h"
#include "Hooks.h"
#include "W2App.h"

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


void BitmapImage::install() {
	addrLoadImgFromDir = Hooks::scanPattern("LoadImgFromDir","\x53\x8B\x5C\x24\x08\x56\x57\x8B\xF1\x56\x8B\xF8\xE8\x00\x00\x00\x00\x85\xC0\x74\x11\x8B\x16\x8B\x40\x04\x8B\x52\x08\x50\x8B\xCE\xFF\xD2\x85\xC0\x75\x2F", "??????xxxxxxx????xxxxxxxxxxxxxxxxxxxxx");
	addrLoadImgFromFile = Hooks::scanPattern("LoadImgFromFile", "\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x83\xEC\x0C\x8B\x44\x24\x20\x56\x68\x00\x00\x00\x00\x50\xC7\x44\x24\x00\x00\x00\x00\x00", "???????xx????xxxx????xxxxxxxxx????xxxx?????");
}
