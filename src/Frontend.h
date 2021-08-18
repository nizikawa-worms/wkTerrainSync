

#ifndef WKTERRAINSYNC_FRONTEND_H
#define WKTERRAINSYNC_FRONTEND_H

#include <utility>
#include <vector>
typedef void CWnd;
typedef void CDataExchange;
typedef unsigned long DWORD;

class Frontend {
public:
	static const int clientMapHelpTextID = 14882137;
	static const int hostMapHelpTextID = 1200;
private:
	static void __stdcall hookFrontendChangeScreen(int screen);
	static inline int currentScreen = 0;
	static inline std::vector<void(__stdcall *)(int)> changeScreenCallbacks;
	static int __fastcall hookGetLocalizedCString(int a1, int id_a2, char** a3);
	static int __fastcall hookMapThumbnailOnMouseMove(CWnd* This, int EDX, int a2, int a3, int a4);
public:
	static inline CWnd *(__stdcall *origAfxSetupStaticText)(CWnd* a1);
	static inline int (__fastcall *origAddEntryToComboBox)(char *str, int EDX, CWnd* a2, int a3);
	static inline void (__stdcall *origWaDDX_Control)(CDataExchange *a1, int a2, CWnd* a3);
	static int setupComboBox(CWnd * box);
	static std::pair<int, int> scalePosition(int x, int y, int realWidth, int realHeight);
	static int scaleSize(int value, int realWidth, int realHeight);

	static int callMessageBox(const char * message, int a2, int a3);
	static inline char* (__stdcall *origGetTextById)(int id);
	static void install();

	static int getCurrentScreen();

	static void registerChangeScreenCallback(void(__stdcall * callback)(int screen));
	static inline int (__stdcall *origPlaySound)(const char * file);
	static void callSetStaticTextValue(CWnd * control, const char * text);
//	static inline int (__fastcall *setHelpText)(CWnd *This, int EDX, int a2, int a3, int a4);
	static inline DWORD * addrCurrentHelpText;
	static inline CWnd *(__fastcall *CWndGetDlgItem)(CWnd *This, int EDX, int nIDDlgItem);
	static inline CWnd * (__stdcall *CWndFromHandle)(int a1);
	static void refreshMapThumbnailHelpText(CWnd * map);
};


#endif //WKTERRAINSYNC_FRONTEND_H
