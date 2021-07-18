#ifndef WKTERRAINSYNC_MISSIONS_H
#define WKTERRAINSYNC_MISSIONS_H


#include <string>
#include <filesystem>
#include <json.hpp>

typedef unsigned long DWORD;

class Missions {
public:
	struct WAMLoaderParams {
		DWORD dword0;
		DWORD trainingNumber;
		DWORD attemptNumber;
		char *path;
		DWORD missionNumber;
		DWORD dword14;
	};
private:
	static inline const std::string missionSchemeName = "[ Default Mission Scheme ]";
	static inline const std::map<int, std::string> alliances = {{0, "Red"}, {1, "Blue"}, {2, "Green"}, {3, "Yellow"}, {4, "Magenta"}, {5, "Cyan"}};
	static inline const std::map<int, std::string> winningConditions = {
			{0, "Kill all CPU controlled teams"},
			{1, "Destroy a vital crate or team. Collecting the vital crate counts as a failure"},
			{2, "Survival: A red-coloured team must be alive when the mission ends"},
			{3, "Mission must be terminated using event or sequence"}};
	static inline const std::map<int, std::string> wormNames = {
			{0, "Sentry"}, {1, "Guard"}, {2, "Sniper"}, {3, "Grenadier"}, {4, "Field Soldier"}, {5, "Artillery"},
			{6, "Captain"}, {7, "Secret Agent"}, {8, "Major"}, {9, "General"}, {10, "Field Marshall"}, {11, "Assassin"},
			{12, "Boggy B"}, {13, "Spadge"}, {14, "Clagnut"},
	};
	static inline const std::map<int, std::string> teamNames = {
			{0, "Cannon Fodder"}, {1, "Commandos"}, {2, "Elite"}, {3, "Enemy"}, {4, "Guardsmen"}, {5, "Gunners"}, {6, "Officers"}, {7, "Patrol"}, {8, "Platoon"}, {9, "Resistance"}, {10, "Special Forces"}
	};

	static inline bool wamFlag = false;
	static inline bool hasCpuTeam = false;
	static inline bool flagInjectingWAM = false;
	static inline std::string wamContents;
	static inline std::filesystem::path wamPath;
	static inline std::string wamDescription;
	static inline std::string wamReplayScheme;
	static inline std::map<int, std::string> wamTeamGroupMap;
	static inline int wamAttemptNumber = 0;
	static inline int copyMissionFilesAttempts = 0;

private:

	static inline unsigned char * lobbyTeamCopy = nullptr;
	static inline int prevNumberOfTeams = 0;

	static inline bool flagDisplayedDescription = false;

	static inline DWORD addrLobbyTeamList = 0;
	static inline char* addrNumberOfTeams = 0;
	static inline DWORD addrAmmoTable = 0;

	static inline DWORD addrPlayerInfoStruct = 0;
	static inline DWORD addrMissionTitleMap = 0;
	static inline DWORD addrMissionDescriptionMap = 0;

	static int __stdcall hookLoadWAMFile(WAMLoaderParams * params);
	static int __stdcall hookCopyTeamsSchemeAndCreateReplay(int a1, const char* a2);
	static int __stdcall hookCopyTeams(int a1);
	static DWORD __fastcall hookSelectFileInComboBox(DWORD This, int EDX, char **a2);
	static bool __stdcall hookCheckGameEndCondition();

	static int __stdcall hookCheckNumberOfRoundsToWinMatch();
	static int __stdcall hookReadWamEvents_patch1_c(char ** buffer, int id, char * group);
	static void hookReadWamEvents_patch1();

	static void saveWamToTmpFile();
	static void clearWamTmpFile();
	static void injectWamAndFixStuff();
	static void setupTeamsInMission(int a1);
	static void setTeamAmmo(int team, int weapon, short int ammo, short int delay);
public:
	static void install();

	static void createMissionDirs();
	static void convertMissionFiles(std::filesystem::path indir, std::filesystem::path outdir);
	static void convertMissionFiles();
	static void convertWAM(std::filesystem::path inpath, std::filesystem::path outdir, std::string prefix);

	static void readWamFile(std::filesystem::path wam);
	static void resetCurrentWam();
	static void setWamContents(std::string);
	static const std::string &getWamContents();


	static const std::string &getWamDescription();

	static void onCreateReplay(nlohmann::json &);
	static void onLoadReplay(nlohmann::json &);

	static void onFrontendExit();
	static void onMapReset();
	static void onExitMapEditor();
	static void onDestroyGameGlobal();


	static std::string describeCurrentWam(bool formatting);

	static void setSkipGoSurrenderAmmo();
	static int getWamAttemptNumber();

	static void setWamAttemptNumber(int wamAttemptNumber);

	static bool getWamFlag();
	static bool getFlagInjectingWam();
};


#endif //WKTERRAINSYNC_MISSIONS_H
