
#ifndef WKTERRAINSYNC_SCHEME_H
#define WKTERRAINSYNC_SCHEME_H

#include <filesystem>
#include <string>
#include <optional>

typedef unsigned long DWORD;
class Scheme {
public:
	static inline const std::string missionDefaultSchemePath = "User/Schemes/[ Default mission scheme ].wsc";
	static inline const std::string missionDefaultSchemeName = "[ Default mission scheme ]";
	static inline const std::string missionCustomSchemeName = "[ Mission-provided scheme ]";

private:
	static inline DWORD addrGetSchemeVersion;
	static inline DWORD addrSchemeStruct = 0;
	static inline DWORD addrSchemeStructAmmo = 0;
	static inline unsigned char * schemeCopy = nullptr;
	static inline bool flagReadingWam = false;

	static int __stdcall hookGetBuiltinSchemeName();
	static int __stdcall hookSetBuiltinScheme(DWORD schemestruct, int id);
	static inline DWORD addrRefreshOfflineMultiplayerSchemeDisplay;
	static inline DWORD addrRefreshOnlineMultiplayerSchemeDisplay;

	static inline int (__stdcall *origSetWscScheme)(DWORD schemestruct, char * path, char flag, bool * out);

	static int __stdcall hookReadWamSchemeSettings();
	static int __stdcall hookReadWamSchemeOptions();
public:
	static void install();

	static std::pair<int, size_t> getSchemeVersionAndSize();
	static DWORD getAddrSchemeStruct();
	static void callSetBuiltinScheme(int id);
	static void loadSchemeFromBytestr(std::string data);
	static std::optional<std::string> saveSchemeToBytestr();
	static void setSchemeName(std::string name);
	static char** callGetBuiltinSchemeName(int id);
	static int callReadWamSchemeSettings(std::string wam);
	static int callReadWamSchemeOptions(std::string wam);
	static void dumpSchemeFromResources(int id, std::filesystem::path path);
	static inline int (__stdcall *origSetBuiltinScheme)(int a1, int schemeid);
	static void setMissionWscScheme(std::filesystem::path wam);
	static inline int (__stdcall *origSendWscScheme)(DWORD a1, char a2);
	static int __stdcall hookSetWscScheme(DWORD schemestruct, char * path, char flag, bool * out);

	static void callRefreshOfflineMultiplayerSchemeDisplay();
	static void callRefreshOnlineMultiplayerSchemeDisplay();
};


#endif //WKTERRAINSYNC_SCHEME_H
