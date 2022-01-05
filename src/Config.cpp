
#include <windows.h>
#include "Config.h"
#include "Utils.h"
#include "WaLibc.h"
#include "Debugf.h"
#include <filesystem>

namespace fs = std::filesystem;

void Config::readConfig() {
	char wabuff[MAX_PATH];
	GetModuleFileNameA(0, (LPSTR)&wabuff, sizeof(wabuff));
	waDir = fs::path(wabuff).parent_path();
	auto inipath = (waDir / iniFile).string();
	moduleEnabled = GetPrivateProfileIntA("general", "EnableModule", 1, inipath.c_str());
	useOffsetCache = GetPrivateProfileIntA("general", "UseOffsetCache", 1, inipath.c_str());
	useCompression = GetPrivateProfileIntA("general", "UseCompression", 1, inipath.c_str());
	useMutex = GetPrivateProfileIntA("general", "UseMutex", 0, inipath.c_str());
	scanTerrainsInBackground = GetPrivateProfileIntA("general", "ScanTerrainsInBackground", 1, inipath.c_str());

	downloadAllowed = GetPrivateProfileIntA("general", "AllowTerrainDownload", 1, inipath.c_str());
	uploadAllowed = GetPrivateProfileIntA("general", "AllowTerrainUpload", 1, inipath.c_str());
	customWaterAllowed = GetPrivateProfileIntA("general", "AllowCustomWater", 1, inipath.c_str());
	spriteOverrideAllowed = GetPrivateProfileIntA("general", "AllowCustomSprites", 1, inipath.c_str());
	additionalParallaxLayersAllowed = GetPrivateProfileIntA("general", "AllowAdditionalParallaxLayers", 1, inipath.c_str());

	ignoreVersionCheck = GetPrivateProfileIntA("general", "IgnoreVersionCheck", 0, inipath.c_str());

	greentextEnabled = GetPrivateProfileIntA("general", "EnableLobbyGreentext", 1, inipath.c_str());
	experimentalMapTypeCheck = GetPrivateProfileIntA("general", "UseExperimentalMapTypeCheck", 1, inipath.c_str());
	messageBoxEnabled = GetPrivateProfileIntA("general", "UseFrontendMessageBox", 1, inipath.c_str());
	nagMessageEnabled = GetPrivateProfileIntA("general", "SendNagMessage", 1, inipath.c_str());
	showInstalledTerrains = GetPrivateProfileIntA("general", "ShowInstalledTerrainsInChat", 0, inipath.c_str());
	thumbnailColor = GetPrivateProfileIntA("general", "MapThumbnailColor", 191, inipath.c_str());
	printMapScaleInChat = GetPrivateProfileIntA("general", "PrintMapScaleInChat", 1, inipath.c_str());
	useCustomTerrainsInSinglePlayerMode = GetPrivateProfileIntA("general", "UseCustomTerrainsInSinglePlayerMode", 1, inipath.c_str());

	parallaxBackA = GetPrivateProfileIntA("parallax", "parallaxBackA", 9011200, inipath.c_str());
	parallaxBackB = GetPrivateProfileIntA("parallax", "parallaxBackB", 42172416, inipath.c_str());
	parallaxFrontA = GetPrivateProfileIntA("parallax", "parallaxFrontA", 65536, inipath.c_str());
	parallaxFrontB = GetPrivateProfileIntA("parallax", "parallaxFrontB", 48003809, inipath.c_str());
	parallaxHideOnBigMaps = GetPrivateProfileIntA("parallax", "HideOnBigMaps", 1, inipath.c_str());

	devConsoleEnabled = GetPrivateProfileIntA("debug", "EnableDevConsole", 0, inipath.c_str());
	hexDumpPackets = GetPrivateProfileIntA("debug", "HexDumpPackets", 0, inipath.c_str());
	replayMsgBox = GetPrivateProfileIntA("debug", "MessageBoxOnReplayLoad", 0, inipath.c_str());
	debugSpriteImg = GetPrivateProfileIntA("debug", "DebugSpriteImgLoading", 0, inipath.c_str());
	logToFile = GetPrivateProfileIntA("debug", "LogToFile", 0, inipath.c_str());
	if(logToFile) {
		logfile = fopen("wkTerrainSync.log", "w");
	}

	storeTerrainFilesInReplay = GetPrivateProfileIntA("replay", "StoreTerrainFilesInReplay", 1, inipath.c_str());
	extractTerrainFromReplaysToTmpDir = GetPrivateProfileIntA("replay", "ExtractTerrainFromReplaysToTmpDir", 1, inipath.c_str());

	superFrontendThumbnailFix = GetPrivateProfileIntA("fixes", "SuperFrontendFixMapThumbnail", 1, inipath.c_str());
	dontRenameSchemeComboBox = GetPrivateProfileIntA("fixes", "DontRenameSchemeComboBox", 0, inipath.c_str());

	dontCreateMissionDirs = GetPrivateProfileIntA("fixes", "DontCreateMissionDirs", 0, inipath.c_str());
	dontConvertMissionFiles = GetPrivateProfileIntA("fixes", "DontConvertMissionFiles", 0, inipath.c_str());

	char buff[2048];
	GetPrivateProfileStringA("exceptions", "ExtendedBackSprLoader", "e27d433fcd8ac2e696f01437525f34c5,", buff, sizeof(buff), inipath.c_str());
	std::string buffstr(buff);
	Utils::tokenizeSet(buffstr, ",", backSprExceptions);
}

bool Config::isDevConsoleEnabled() {
	return devConsoleEnabled;
}

bool Config::isModuleEnabled() {
	return moduleEnabled;
}

bool Config::isDownloadAllowed() {
	return downloadAllowed;
}

bool Config::isUploadAllowed() {
	return uploadAllowed;
}

void Config::setDownloadAllowed(bool downloadAllowed) {
	Config::downloadAllowed = downloadAllowed;
}

void Config::setUploadAllowed(bool uploadAllowed) {
	Config::uploadAllowed = uploadAllowed;
}


//StepS tools
typedef unsigned long long QWORD;
#define MAKELONGLONG(lo,hi) ((LONGLONG(DWORD(lo) & 0xffffffff)) | LONGLONG(DWORD(hi) & 0xffffffff) << 32 )
#define QV(V1, V2, V3, V4) MAKEQWORD(V4, V3, V2, V1)
#define MAKEQWORD(LO2, HI2, LO1, HI1) MAKELONGLONG(MAKELONG(LO2,HI2),MAKELONG(LO1,HI1))
QWORD GetModuleVersion(HMODULE hModule)
{
	char WApath[MAX_PATH]; DWORD h;
	GetModuleFileNameA(hModule,WApath,MAX_PATH);
	DWORD Size = GetFileVersionInfoSizeA(WApath,&h);
	if(Size)
	{
		void* Buf = malloc(Size);
		GetFileVersionInfoA(WApath,h,Size,Buf);
		VS_FIXEDFILEINFO *Info; DWORD Is;
		if(VerQueryValueA(Buf, "\\", (LPVOID*)&Info, (PUINT)&Is))
		{
			if(Info->dwSignature==0xFEEF04BD)
			{
				return MAKELONGLONG(Info->dwFileVersionLS, Info->dwFileVersionMS);
			}
		}
		free(Buf);
	}
	return 0;
}

int Config::waVersionCheck() {
	if(ignoreVersionCheck)
		return 1;

	auto version = GetModuleVersion((HMODULE)0);
	char versionstr[64];
	_snprintf_s(versionstr, _TRUNCATE, "Detected game version: %u.%u.%u.%u", PWORD(&version)[3], PWORD(&version)[2], PWORD(&version)[1], PWORD(&version)[0]);
	debugf("%s\n", versionstr);

	std::string tversion = getFullStr();
	char buff[512];
	if (version < QV(3,8,0,0)) {
		_snprintf_s(buff, _TRUNCATE, "wkTerrainSync is not compatible with WA versions older than 3.8.0.0.\n\n%s", versionstr);
		MessageBoxA(0, buff, tversion.c_str(), MB_OK | MB_ICONERROR);
		return 0;
	}
	if (version >= QV(3,9,0,0)) {
		_snprintf_s(buff, _TRUNCATE, "wkTerrainSync is not compatible with WA versions 3.9.x.x and newer. Besides, this functionality should be built into WA by now...\n\n%s", versionstr);
		MessageBoxA(0, buff, tversion.c_str(), MB_OK | MB_ICONERROR);
		return 0;
	}
	if(version == QV(3,8,0,0) || version == QV(3,8,1,0)) {
		return 1;
	}

	_snprintf_s(buff, _TRUNCATE, "wkTerrainSync is not designed to work with your WA version and may malfunction.\n\nTo disable this warning set IgnoreVersionCheck=1 in wkTerrainSync.ini file.\n\n%s", versionstr);
	return MessageBoxA(0, buff, tversion.c_str(), MB_OKCANCEL | MB_ICONWARNING) == IDOK;
}

void Config::addVersionInfoToJson(nlohmann::json & json) {
	json["module"] = getModuleStr();
	json["version"] = getVersionStr();
	json["versionInt"] = getVersionInt();
	json["build"] = getBuildStr();
	json["protocol"] = getProtocolVersion();
}

std::string Config::getModuleStr() {
	return "wkTerrainSync";
}

std::string Config::getVersionStr() {
	return "v1.2.4";
}

int Config::getVersionInt() {
	return 1020400;
}

std::string Config::getBuildStr() {
	return __DATE__ " " __TIME__;
}

std::string Config::getFullStr() {
	return getModuleStr() + " " + getVersionStr() + " (build: " + getBuildStr() + ")";
}

int Config::getParallaxFrontA() {
	return parallaxFrontA;
}

int Config::getParallaxFrontB() {
	return parallaxFrontB;
}

int Config::getParallaxBackA() {
	return parallaxBackA;
}

int Config::getParallaxBackB() {
	return parallaxBackB;
}

bool Config::isGreentextEnabled() {
	return greentextEnabled;
}

bool Config::isExperimentalMapTypeCheck() {
	return experimentalMapTypeCheck;
}

bool Config::isMessageBoxEnabled() {
	return messageBoxEnabled;
}

bool Config::isHexDumpPacketsEnabled() {
	return hexDumpPackets;
}

const std::set<std::string> &Config::getBackSprExceptions() {
	return backSprExceptions;
}

bool Config::isNagMessageEnabled() {
	return nagMessageEnabled;
}

int Config::getParallaxHideOnBigMaps() {
	return parallaxHideOnBigMaps;
}

bool Config::isShowInstalledTerrainsEnabled() {
	return showInstalledTerrains;
}

int Config::getThumbnailColor() {
	return thumbnailColor;
}

bool Config::isSuperFrontendThumbnailFix() {
	return superFrontendThumbnailFix;
}

bool Config::isPrintMapScaleInChat() {
	return printMapScaleInChat;
}

bool Config::isCustomWaterAllowed() {
	return customWaterAllowed;
}

bool Config::isSpriteOverrideAllowed() {
	return spriteOverrideAllowed;
}

bool Config::isAdditionalParallaxLayersAllowed() {
	return additionalParallaxLayersAllowed;
}

bool Config::isDontRenameSchemeComboBox() {
	return dontRenameSchemeComboBox;
}

const std::filesystem::path &Config::getWaDir() {
	return waDir;
}

int Config::getProtocolVersion() {
	return 0;
}


bool Config::isDontCreateMissionDirs() {
	return dontCreateMissionDirs;
}

bool Config::isDontConvertMissionFiles() {
	return dontConvertMissionFiles;
}

int Config::getModuleInitialized() {
	return moduleInitialized;
}

void Config::setModuleInitialized(int moduleInitialized) {
	Config::moduleInitialized = moduleInitialized;
	if(moduleInitialized) {
		for(auto & cb : moduleInitializedCallbacks) {
			cb();
		}
	}
}

void Config::registerModuleInitializedCallback(void(__stdcall * callback)()) {
	moduleInitializedCallbacks.push_back(callback);
}

bool Config::isUseCustomTerrainsInSinglePlayerMode() {
	return useCustomTerrainsInSinglePlayerMode;
}

bool Config::isUseOffsetCache() {
	return useOffsetCache;
}

std::string Config::getWaVersionAsString() {
	char buff[32];
	auto version = GetModuleVersion(0);
	sprintf_s(buff, "%u.%u.%u.%u", PWORD(&version)[3], PWORD(&version)[2], PWORD(&version)[1], PWORD(&version)[0]);
	return buff;
}

bool Config::isReplayMsgBox() {
	return replayMsgBox;
}

bool Config::isUseCompression() {
	return useCompression;
}

bool Config::isDebugSpriteImg() {
	return debugSpriteImg;
}

bool Config::isLogToFile() {
	return logToFile;
}

bool Config::isScanTerrainsInBackground() {
	return scanTerrainsInBackground;
}

bool Config::isStoreTerrainFilesInReplay() {
	return storeTerrainFilesInReplay;
}

bool Config::isMutexEnabled() {
	return useMutex;
}

bool Config::isExtractTerrainFromReplaysToTmpDir() {
	return extractTerrainFromReplaysToTmpDir;
}
