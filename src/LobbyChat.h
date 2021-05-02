
#ifndef WKTERRAINSYNC_LOBBYCHAT_H
#define WKTERRAINSYNC_LOBBYCHAT_H

#include <string>

typedef unsigned long       DWORD;
class LobbyChat {
private:
	static inline DWORD lobbyHostScreen = 0;
	static inline DWORD lobbyClientScreen = 0;
	static inline const std::string terrainNagMessage = \
			"SYS::ALL:"\
			"- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - \n"\
			"The host is using wkTerrainSync to play maps with custom terrains.\n"\
			"You won't be able to play in this lobby unless you install the latest version of wkTerrainSync.\n\n"\
			\
			"The latest version of wkTerrainSync can be obtained at: https://worms2d.info/WkTerrainSync\n"\
			"- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -";

	static inline const std::string bigMapNagMessage = \
			"SYS::ALL:"\
			"- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - \n"\
			"The host is using wkTerrainSync to play on custom-sized maps.\n"\
			"You won't be able to play in this lobby unless you install the latest version of wkTerrainSync.\n\n"\
			\
			"The latest version of wkTerrainSync can be obtained at: https://worms2d.info/WkTerrainSync\n"\
			"- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -";

	static inline const std::string helpMessage = \
			"available commands:\n" \
					"\t/terrains - send your wkTerrainSync version\n" \
					"\t/terrains list - send your wkTerrainSync version and a list of installed terrains\n" \
					"\t/scale - show current map scale";

	static int __stdcall hookConstructLobbyHostScreen(int a1, int a2);
	static int __fastcall hookDestructLobbyHostScreen(void *This, int EDX, char a2);
	static int __stdcall hookConstructLobbyHostEndScreen(DWORD a1, unsigned int a2, char a3, int a4);
	static int __stdcall hookDestructLobbyHostEndScreen(int a1);

	static int __stdcall hookConstructLobbyClientScreen(int a1, int a2);
	static int __fastcall hookDestructLobbyClientScreen(void *This, int EDX, char a2);
	static int __stdcall hookConstructLobbyClientEndScreen(DWORD a1);
	static void __fastcall hookDestructCWnd(int This);

	static int __fastcall hookLobbyClientCommands(void *This, void *EDX, char **commandstrptr, char **argstrptr);
	static int __stdcall hookLobbyDisplayMessage(int a1, char *msg);

	static inline DWORD addrConstructLobbyHostScreen;
	static inline DWORD addrConstructLobbyHostEndScreen;
	static inline DWORD addrConstructLobbyHostEndScreenWrapper;
public:
	static void install();
	static void lobbyPrint(char * msg);
//	static void resetLobbyScreenPtrs();

	static const std::string &getTerrainNagMessage();
	static const std::string &getBigMapNagMessage();

	static DWORD getAddrConstructLobbyHostScreen();
	static DWORD getAddrConstructLobbyHostEndScreen();
	static DWORD getAddrConstructLobbyHostEndScreenWrapper();

	static DWORD getLobbyHostScreen();
	static DWORD getLobbyClientScreen();
};


#endif //WKTERRAINSYNC_LOBBYCHAT_H
