
#ifndef WKTERRAINSYNC_BITMAPIMAGE_H
#define WKTERRAINSYNC_BITMAPIMAGE_H

typedef unsigned long DWORD;
class BitmapImage {
private:
	static BitmapImage * __stdcall hookConstructBitmapImage(int width);
	static BitmapImage * __stdcall hookLoadImgFromDir(int a3);
	static BitmapImage * __stdcall hookLoadImgFromFile(int a2, char *filename);
public:
	int vtable_dword0; // 0x0
	int unknown4; // 0x4
	int data_dword8; // 0x8
	int bitdepth_dwordC; // 0xC
	int rowsize_dword10; // 0x10
	int max_width_dword14; // 0x14
	int max_height_dword18; // 0x18
	int current_offsetX_dword1C; // 0x1C
	int current_offsetY_dword20; // 0x20
	int current_width_dword24; // 0x24
	int current_height_dword28; // 0x28
	// values below are probably unused / wrong size of structure
	int unknown2C; // 0x2C
	int unknown30; // 0x30
	int unknown34; // 0x34
	int unknown38; // 0x38
	int unknown3C; // 0x3C
	int unknown40; // 0x40
	int unknown44; // 0x44
	int unknown48; // 0x48

	static BitmapImage * callLoadImgFromDir(char * filename, DWORD a2, DWORD a3);
	static BitmapImage* callLoadImgFromFile(DWORD DD_Display, DWORD Palette, char * path);
	static void install();
};

#endif //WKTERRAINSYNC_BITMAPIMAGE_H
