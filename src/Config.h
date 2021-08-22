#ifndef WKTERRAINSYNC_CONFIG_H
#define WKTERRAINSYNC_CONFIG_H

#include <string>
#include <json.hpp>
#include <set>
#include <filesystem>

class Config {
public:
	static inline const std::string iniFile = "wkTerrainSync.ini";
	static inline const std::string moduleName = "wkTerrainSync";

private:
	static inline bool moduleEnabled = true;
	static inline bool devConsoleEnabled = true;

	static inline bool downloadAllowed = true;
	static inline bool uploadAllowed = true;
	static inline bool ignoreVersionCheck = false;
	static inline bool greentextEnabled = true;
	static inline bool experimentalMapTypeCheck = true;
	static inline bool hexDumpPackets = false;
	static inline bool nagMessageEnabled = true;
	static inline bool showInstalledTerrains = false;
	static inline bool superFrontendThumbnailFix = true;
	static inline bool printMapScaleInChat = true;
	static inline bool customWaterAllowed = true;
	static inline bool spriteOverrideAllowed = true;
	static inline bool additionalParallaxLayersAllowed = true;
	static inline bool dontRenameSchemeComboBox = false;
	static inline bool dontCreateMissionDirs = false;
	static inline bool dontConvertMissionFiles = false;
	static inline bool useCustomTerrainsInSinglePlayerMode = false;
	static inline bool useOffsetCache = true;
	static inline bool replayMsgBox = false;

	static inline std::filesystem::path waDir;

	static inline std::vector<void(__stdcall *)()> moduleInitializedCallbacks;
	static inline int moduleInitialized;

public:
	static bool isNagMessageEnabled();

private:
	static inline int parallaxFrontA;
	static inline int parallaxFrontB;
	static inline int parallaxBackA;
	static inline int parallaxBackB;
	static inline int parallaxHideOnBigMaps;
	static inline int thumbnailColor;
	static inline bool messageBoxEnabled = true;
	static inline std::set<std::string> backSprExceptions;
public:
	static const std::set<std::string> &getBackSprExceptions();

public:
	static void readConfig();
	static bool isModuleEnabled();
	static bool isDevConsoleEnabled();

	static bool isDownloadAllowed();
	static bool isUploadAllowed();

	static void setDownloadAllowed(bool downloadAllowed);
	static void setUploadAllowed(bool uploadAllowed);
	static int waVersionCheck();

	static std::string getVersionStr();
	static std::string getBuildStr();
	static std::string getModuleStr();
	static std::string getFullStr();
	static int getProtocolVersion();

	static int getParallaxFrontA();
	static int getParallaxFrontB();
	static int getParallaxBackA();
	static int getParallaxBackB();
	static int getParallaxHideOnBigMaps();

	static int getThumbnailColor();

	static void addVersionInfoToJson(nlohmann::json &);

	static bool isGreentextEnabled();
	static bool isExperimentalMapTypeCheck();

	static bool isMessageBoxEnabled();
	static bool isHexDumpPacketsEnabled();

	static bool isShowInstalledTerrainsEnabled();
	static bool isSuperFrontendThumbnailFix();

	static bool isPrintMapScaleInChat();
	static bool isCustomWaterAllowed();
	static bool isSpriteOverrideAllowed();
	static bool isAdditionalParallaxLayersAllowed();
	static bool isDontRenameSchemeComboBox();
	static bool isUseCustomTerrainsInSinglePlayerMode();
	static bool isReplayMsgBox();

	static const std::filesystem::path &getWaDir();

	static bool isDontCreateMissionDirs();
	static bool isDontConvertMissionFiles();

	static bool isUseOffsetCache();
	static std::string getWaVersionAsString();

	static int getModuleInitialized();
	static void setModuleInitialized(int moduleInitialized);

	static void registerModuleInitializedCallback(void(__stdcall * callback)());
};


#endif //WKTERRAINSYNC_CONFIG_H
