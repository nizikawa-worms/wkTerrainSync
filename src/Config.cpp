
#include <Windows.h>
#include "Config.h"
#include "Utils.h"


void Config::readConfig() {
	moduleEnabled = GetPrivateProfileIntA("general", "EnableModule", 1, iniFile.c_str());

	downloadAllowed = GetPrivateProfileIntA("general", "AllowTerrainDownload", 1, iniFile.c_str());
	uploadAllowed = GetPrivateProfileIntA("general", "AllowTerrainUpload", 1, iniFile.c_str());
	ignoreVersionCheck = GetPrivateProfileIntA("general", "IgnoreVersionCheck", 0, iniFile.c_str());

	greentextEnabled = GetPrivateProfileIntA("general", "EnableLobbyGreentext", 1, iniFile.c_str());
	experimentalMapTypeCheck = GetPrivateProfileIntA("general", "UseExperimentalMapTypeCheck", 1, iniFile.c_str());
	messageBoxEnabled = GetPrivateProfileIntA("general", "UseFrontendMessageBox", 1, iniFile.c_str());
	nagMessageEnabled = GetPrivateProfileIntA("general", "SendNagMessage", 1, iniFile.c_str());

	parallaxBackA = GetPrivateProfileIntA("parallax", "parallaxBackA", 9011200, iniFile.c_str());
	parallaxBackB = GetPrivateProfileIntA("parallax", "parallaxBackB", 42172416, iniFile.c_str());
	parallaxFrontA = GetPrivateProfileIntA("parallax", "parallaxFrontA", 65536, iniFile.c_str());
	parallaxFrontB = GetPrivateProfileIntA("parallax", "parallaxBackB", 48003809, iniFile.c_str());

	devConsoleEnabled = GetPrivateProfileIntA("debug", "EnableDevConsole", 1, iniFile.c_str());
	hexDumpPackets = GetPrivateProfileIntA("debug", "HexDumpPackets", 0, iniFile.c_str());

	char buff[2048];
	GetPrivateProfileStringA("exceptions", "ExtendedBackSprLoader", "e27d433fcd8ac2e696f01437525f34c5,", buff, sizeof(buff), iniFile.c_str());
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
	sprintf_s(versionstr, "Detected game version: %u.%u.%u.%u", PWORD(&version)[3], PWORD(&version)[2], PWORD(&version)[1], PWORD(&version)[0]);
	printf("%s\n", versionstr);

	std::string tversion = getFullStr();
	char buff[512];
	if (version < QV(3,8,0,0)) {
		sprintf_s(buff, "wkTerrainSync is not compatible with WA versions older than 3.8.0.0.\n\n%s", versionstr);
		MessageBoxA(0, buff, tversion.c_str(), MB_OK | MB_ICONERROR);
		return 0;
	}
	if (version >= QV(3,9,0,0)) {
		sprintf_s(buff, "wkTerrainSync is not compatible with WA versions 3.9.x.x and newer. Besides, this functionality should be built into WA by now...\n\n%s", versionstr);
		MessageBoxA(0, buff, tversion.c_str(), MB_OK | MB_ICONERROR);
		return 0;
	}
	if(version == QV(3,8,0,0) || version == QV(3,8,1,0)) {
		return 1;
	}

	sprintf_s(buff, "wkTerrainSync is not designed to work with your WA version and may malfunction.\n\nTo disable this warning set IgnoreVersionCheck=1 in wkTerrainSync.ini file.\n\n%s", versionstr);
	return MessageBoxA(0, buff, tversion.c_str(), MB_OKCANCEL | MB_ICONWARNING) == IDOK;
}

void Config::addVersionInfoToJson(nlohmann::json & json) {
	json["module"] = getModuleStr();
	json["version"] = getVersionStr();
	json["build"] = getBuildStr();
}

std::string Config::getModuleStr() {
	return "wkTerrainSync";
}
std::string Config::getVersionStr() {
	return "v1.0.2";
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

