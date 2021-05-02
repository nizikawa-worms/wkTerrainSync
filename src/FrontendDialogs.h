
#ifndef WKTERRAINSYNC_FRONTENDDIALOGS_H
#define WKTERRAINSYNC_FRONTENDDIALOGS_H

#define _X86_
#include <windef.h>

typedef void CWnd;
typedef void CDataExchange;

class FrontendDialogs {
public:
	static const int comboWidthID = 0x2137;
	static const int comboHeightID = 0x2138;
private:
	static inline HWND comboWidthHwnd;
	static inline HWND comboHeightHwnd;
	static inline CWnd * comboHeight = nullptr;
	static inline CWnd * comboWidth = nullptr;
	static inline bool comboMoved = false;
	static inline int windowWidth = 0;
	static inline int windowHeight = 0;
	static inline int moveMapComboLeft = 0;

	static void __fastcall hookLocalMultiplayerDDX_Control(CWnd* This, int EDX, CDataExchange *a2);
	static void __fastcall hookLobbyHostScreenDDX_Control(CWnd* This, int EDX, CDataExchange *a2);
	static void __fastcall hookLobbyHostEndScreenWrapperDDX_Control(CWnd* This, int EDX, CDataExchange *a2);
	static void __fastcall hookLocalMultiplayerEndscreenDDX_Control(CWnd* This, int EDX, CDataExchange *a2);

	static LRESULT __stdcall hookWidthWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT __stdcall hookHeightWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static void createTerrainSizeComboBoxes(CWnd * window, CWnd * mapList, CDataExchange * dataExchange, int startX, int startY, int moveMapCombo=0);

	static int __stdcall hookAddDefaultLevelsToCombo();
public:
	static void install();
	static void destroyTerrainSizeComboBoxes();
	static void resetComboBoxesText();
};


#endif //WKTERRAINSYNC_FRONTENDDIALOGS_H
