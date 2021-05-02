

#ifndef WKTERRAINSYNC_FRONTEND_H
#define WKTERRAINSYNC_FRONTEND_H

#include <utility>

typedef void CWnd;
typedef void CDataExchange;

class Frontend {
private:
	static void __stdcall hookChangeScreen(int screen);
public:
	static inline CWnd *(__stdcall *origAfxSetupStaticText)(CWnd* a1);
	static inline int (__fastcall *origAddEntryToComboBox)(char *str, int EDX, CWnd* a2, int a3);
	static inline void (__stdcall *origWaDDX_Control)(CDataExchange *a1, int a2, CWnd* a3);
	static int setupComboBox(CWnd * box);
	static std::pair<int, int> scalePosition(int x, int y, int realWidth, int realHeight);
	static int scaleSize(int value, int realWidth, int realHeight);

	static int callMessageBox(char * message, int a2, int a3);
	static void install();
};


#endif //WKTERRAINSYNC_FRONTEND_H
