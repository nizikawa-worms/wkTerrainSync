
#ifndef WKTERRAINSYNC_SPRITES_H
#define WKTERRAINSYNC_SPRITES_H


#include <vector>
#include <string>
#include "BitmapImage.h"

typedef unsigned short       WORD;
class Sprites {
public:
	static const int back2SprId = 698;
	static const int frontSprId = 699;
	static const int waterSprId = 620;

	class SpriteFrame {
	public:
		DWORD bitmapoffset;
		WORD startx;
		WORD starty;
		WORD endx;
		WORD endy;
	};


public:
	DWORD dword0;
	DWORD dword4;
	WORD word8;
	WORD fps_wordA;
	WORD width_wordC;
	WORD height_wordE;
	WORD flags_word10;
	WORD max_frames_copy_word12;
	WORD word14;
	WORD max_frames_word16;
	WORD word18;
	WORD word1A;
	DWORD dword1C;
	DWORD dword20;
	DWORD dword24;
	SpriteFrame * frame_meta_ptr_dword28;
	DWORD dword2C;
	WORD word30;
	WORD word32;


	DWORD bitmap_dword34;

	DWORD dword38;
	DWORD dword3C;
	DWORD dword40;
	DWORD dword44;
	DWORD dword48;
	DWORD dword4C;
	DWORD dword50;
	DWORD dword54;
	DWORD dword58;
	DWORD dword5C;

//	Bitmap::BitmapImage bitmap;
	DWORD dword60;
	DWORD probably_sprite_bitmap_data_dword64;
	DWORD maybe_palette_data_dword68;
	DWORD dword6C;

private:
	static inline int customFillColor = 0;

	static DWORD __fastcall hookLoadSpriteFromVFS(DWORD DD_Display, int EDX, int palette, int index, int a4, int vfs_a5, const char *filename);
	static DWORD __fastcall hookLoadSpriteFromTerrain(DWORD DDDisplay, DWORD edx, int palette_num_a2, int sprite_id_a3, int vfs_a4, const char *filename);
	static int __fastcall hookDrawBackSprite(int *a1, int a2, int a3, int a4, int a5, int a6, int sprite_a7, int a8);

public:
	static int install();

	static int getCustomFillColor();
	static void setCustomFillColor(int customFillColor);
	static int callCheckIfFileExistsInVFS(const char * filename, DWORD vfs);
};


#endif //WKTERRAINSYNC_SPRITES_H
