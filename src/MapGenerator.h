
#ifndef WKTERRAINSYNC_MAPGENERATOR_H
#define WKTERRAINSYNC_MAPGENERATOR_H


#include <json.hpp>

typedef unsigned long DWORD;
class BitmapImage;
class MapGenerator {
public:
	static constexpr float scaleIncrement = 0.1;
	static constexpr float scaleMax = 5.0;
	static const int magicMapType = 237;
	enum extraBits {f_Flag0=1, f_Flag2=2, f_Flag3=4, f_Flag4=8, f_Flag5=16, f_Flag6=32, f_DecorHell=64, f_Flag8=128};
private:
	static inline bool flagResizeBitmap = false;
	static inline int countResizeBitmap = 0;

	static inline int scaleXIncrements = 0;
	static inline int scaleYIncrements = 0;
	static inline unsigned int flagExtraFeatures = 0;

	static inline DWORD mapThumbnailPtr = 0;
	static inline DWORD mapTypeCheckThis = 0;

	static int __stdcall hookGenerateMapFromParams(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12);
	static int __stdcall hookEditorGenerateMapVariant(int a1, int cavern, int variant);
	static int __stdcall hookConstructMapThumbnailCWnd(DWORD a1, char a2);
	static int __fastcall hookDestructMapThumbnailCWnd(DWORD This);

	static int __stdcall hookSaveRandomMapAsIMG(char *filename, int seed, int cavern, int variant);
	static int __stdcall hookMapPreviewScaling();

	static void __stdcall hookEditorGenerateMap_patch1_c(BitmapImage* src, BitmapImage* thumb);
	static void hookEditorGenerateMap_patch1();
	static void hookBakeMap_patch1();
	static void __stdcall hookBakeMap_patch1_c(DWORD ebp);
	static void hookCreateBitThumbnail_patch1();
	static int __stdcall hookCreateBitThumbnail_patch1_c(int a1, int a2, int a3, int a4);
	static int __stdcall hookW2PrvToEditor_patch1_c(int a1, int a2, int a3, int a4);
	static void hookW2PrvToEditor_patch1();
	static int __stdcall hookExitMapEditor(int a1);

	static inline std::vector<void(__stdcall *)(int)> onMapResetCallbacks;
	static inline std::vector<void(__stdcall *)()> onMapEditorExitCallbacks;
public:

	static void onTerrainPacket(DWORD This, DWORD EDX, unsigned char * packet, size_t size);
	static void onMapTypeChecks(int a1);
	static void onConstructBitmapImage(int & width, int &height, int & bitdepth);
	static void onReseedAndWriteMapThumbnail(int a1);
	static void install();

	static void setScaleX(int scaleX, bool resetCombo = true);
	static void setScaleY(int scaleY,  bool resetCombo = true);
	static void resetScale(bool writethumb=true);

	static unsigned int getFlagExtraFeatures();
	static void setFlagExtraFeatures(unsigned int flagExtraFeatures);

	static void onFrontendExit();

	static void writeMagicMapTypeToThumbnail(int *mtype);
	static unsigned int encodeMagicMapType();

	static void onCreateReplay(nlohmann::json &);
	static void onLoadReplay(nlohmann::json &);

	static int getScaleXIncrements();
	static int getScaleYIncrements();
	static float getEffectiveScaleX();
	static float getEffectiveScaleY();
	static int getEffectiveWidth(int width=1920);
	static int getEffectiveHeight(int height=696);

	static void printCurrentScale();

	static void registerOnMapResetCallback(void(__stdcall * callback)(int reason));
	static void registerOnMapEditorExitCallback(void(__stdcall * callback)());
};


#endif //WKTERRAINSYNC_MAPGENERATOR_H
