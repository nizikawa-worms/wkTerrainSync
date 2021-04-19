
#ifndef WKTERRAINSYNC_LOBBYCHAT_H
#define WKTERRAINSYNC_LOBBYCHAT_H

#include <string>

typedef unsigned long       DWORD;
class LobbyChat {
private:
	static inline DWORD lobbyHostScreen = 0;
	static inline DWORD lobbyClientScreen = 0;
	static inline const std::string nagMessage = \
			"SYS::ALL:"\
			"- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - \n"\
			"The host is using wkTerrainSync to play maps with custom terrains.\n"\
			"You won't be able to play in this lobby unless you install the latest version of wkTerrainSync.\n\n"\
			\
			"The latest version of wkTerrainSync can be obtained at: https://worms2d.info/WkTerrainSync\n"\
			"- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -";

	static int __stdcall hookConstructLobbyHostScreen(int a1, int a2);
	static int __stdcall hookConstructLobbyHostEndScreen(DWORD a1, unsigned int a2, char a3, int a4);
	static int __stdcall hookDestructLobbyHostEndScreen(int a1);

	static int __stdcall hookConstructLobbyClientScreen(int a1, int a2);
	static int __stdcall hookConstructLobbyClientEndScreen(DWORD a1);

	static int __fastcall hookLobbyClientCommands(void *This, void *EDX, char **commandstrptr, char **argstrptr);
	static int __stdcall hookLobbyDisplayMessage(int a1, char *msg);
public:
	static void install();
	static void lobbyPrint(char * msg);
	static void resetLobbyScreenPtrs();

	static const std::string &getNagMessage();
};


#endif //WKTERRAINSYNC_LOBBYCHAT_H
