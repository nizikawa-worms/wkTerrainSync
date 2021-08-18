
#include "Missions.h"
#include "Hooks.h"
#include "WaLibc.h"
#include "Utils.h"
#include "Frontend.h"
#include "Protocol.h"
#include <filesystem>
#include <fstream>
#include <Base64.h>
#include "LobbyChat.h"
#include "Packets.h"
#include "Replay.h"
#include "Config.h"
#include "CPUflags.h"
#include "Scheme.h"
#include "Debugf.h"

namespace fs = std::filesystem;

std::map<int, const unsigned char *> cpuflags = {
		{1, CPU1}, {2, CPU2}, {3, CPU3}, {4, CPU4}, {5, CPU5}, {6, CPU6}, {7, CPU7}
};

int (__stdcall *origLoadWAMFile)(Missions::WAMLoaderParams * params);
int __stdcall Missions::hookLoadWAMFile(WAMLoaderParams * params) {
	return origLoadWAMFile(params);
}

int (__stdcall *origCopyTeamsSchemeAndCreateReplay)(int a1, const char* a2);
int __stdcall Missions::hookCopyTeamsSchemeAndCreateReplay(int a1, const char* a2) {
	if(wamFlag && !flagInjectingWAM) {
		saveWamToTmpFile();
		Replay::setFlagExitEarly(true);
	}
	int ret = origCopyTeamsSchemeAndCreateReplay(a1, a2);
	if(wamFlag) {
		if (!flagInjectingWAM) {
			injectWamAndFixStuff();
			Replay::setFlagExitEarly(false);
//			*(DWORD *)(a1 + 55828) = 1; //winning condition
			*(char*)(a1 + 56) = 0; // set round number to 0
			std::string prefix = std::format("{} Custom Mission", a2);
			Replay::hookCreateReplay(a1, prefix.c_str(), Replay::getLastTime());
			Scheme::callSetBuiltinScheme(2); //restore scheme to intermediate to fix bug with WAM running after rematch
			if(lobbyTeamCopy) {
				// restore team list in lobby so it's not overwritten by teams from WAM
				memcpy((void*)addrLobbyTeamList, (void*)lobbyTeamCopy, 0xD7B * 6);
				free(lobbyTeamCopy);
				lobbyTeamCopy = nullptr;
			}
		}

	}
	return ret;
}

void Missions::injectWamAndFixStuff() {
	flagInjectingWAM = true;
	struct WAMLoaderParams params;
	memset(&params, 0, sizeof(params));

	params.path = (char *) WaLibc::waMalloc(wamPath.string().size());
	strcpy(params.path, wamPath.string().c_str());
	params.attemptNumber = wamAttemptNumber;
	hookLoadWAMFile(&params);
	setSkipGoSurrenderAmmo();
	clearWamTmpFile();
	flagInjectingWAM = false;
}

void Missions::setSkipGoSurrenderAmmo() {
	for(int i=0; i < 6; i++) {
		//give every team access to skipgo & surrender
		setTeamAmmo(i, 57, -1, 0);
		setTeamAmmo(i, 58, -1, 0);
	}
	// or let the wam decide
	for(auto & it : wamTeamGroupMap) {
		auto & group = it.second;
		auto & team = it.first;
		short int ammo_skipgo = GetPrivateProfileIntA(group.c_str(), "Ammo_SkipGo", -1, wamPath.string().c_str());
		short int delay_skipgo = GetPrivateProfileIntA(group.c_str(), "Delay_SkipGo", 0, wamPath.string().c_str());
		short int ammo_surrender = GetPrivateProfileIntA(group.c_str(), "Ammo_Surrender", -1, wamPath.string().c_str());
		short int delay_surrender = GetPrivateProfileIntA(group.c_str(), "Delay_Surrender", 0, wamPath.string().c_str());
		setTeamAmmo(team, 57, ammo_skipgo, delay_skipgo);
		setTeamAmmo(team, 58, ammo_surrender, delay_surrender);
	}
}

void Missions::saveWamToTmpFile() {
	wamPath = std::tmpnam(nullptr);
	std::ofstream out(wamPath, std::ios::binary);
	if(!out.good()) {
		char buff[256];
		_snprintf_s(buff, _TRUNCATE, "Failed to save WAM file as: %s", wamPath.string().c_str());
		Frontend::callMessageBox(buff, 0, 0);
		resetCurrentWam();
	} else {
		out.write(wamContents.data(), wamContents.size());
		out.close();
		debugf("wampath: %s\n", wamPath.string().c_str());
	}
}
void Missions::clearWamTmpFile() {
	if(wamPath.empty()) return;
	unlink(wamPath.string().c_str());
	wamPath.clear();
}


int (__stdcall *origCopyTeams)(int a1);
int __stdcall Missions::hookCopyTeams(int a1) {
	if(Replay::isReplayPlaybackFlag() && wamFlag) {
		char* playerinfo = (char*)addrPlayerInfoStruct;
		for(int i=0; i<13; i++) {
			if(!playerinfo[116]) {playerinfo[116] = 1;} // fix for checksum error in replay playback
			playerinfo += 120;
		}
		if(!wamReplayScheme.empty()) {
			Scheme::loadSchemeFromBytestr(wamReplayScheme);
		}
	}

	if(!wamFlag) {
		return origCopyTeams(a1);
	} else {
		if(!flagInjectingWAM) {
			prevNumberOfTeams = *addrNumberOfTeams;
			lobbyTeamCopy = (unsigned char*)malloc(0xD7B * 6);
			memcpy((void*)lobbyTeamCopy, (void*)addrLobbyTeamList, 0xD7B * 6); //backup player teams in lobby
			return origCopyTeams(a1);
		} else {
			if(lobbyTeamCopy) {
				memcpy((void*)addrLobbyTeamList, lobbyTeamCopy, 0xD7B * 6);
			}
			setupTeamsInMission(0);
			return origCopyTeams(a1);
		}
	}
}

void Missions::setupTeamsInMission(int a1) {
	std::string wp = wamPath.string();
	const char * wpcstr = wp.c_str();
	char count = 0;
	int numPlaceholders = 0;
	hasCpuTeam = false;
	wamTeamGroupMap.clear();
	int multiplayer = GetPrivateProfileIntA("Mission", "Multiplay", 0, wpcstr);
	for(int i=0; i < 6; i++) {
		DWORD teamMeta = addrLobbyTeamList + i * 0xD7B;
		char group[32];
		_snprintf_s(group, _TRUNCATE, "CPUTeam%d", i);
		if(i == 0)
			strcpy_s(group, "HumanTeam");
		unsigned int numworms = GetPrivateProfileIntA(group, "NumberOfWorms", 0, wpcstr);
		if(multiplayer && !numworms){
			_snprintf_s(group, _TRUNCATE, "HumanTeam%d", i);
			numworms = GetPrivateProfileIntA(group, "NumberOfWorms", 0, wpcstr);
			if(!numworms) {
				// fallback for multiplayer mission files without NumberOfWorms, but with Worm1_Energy
				for(int a=1; a <=8; a++) {
					char worm[32];
					_snprintf_s(worm, _TRUNCATE, "Worm%d_Energy", a);
					if(!GetPrivateProfileIntA(group, worm, 0, wpcstr)) break;
					numworms++;
				}
			}
		}
		if(numworms > 8) numworms = 0;
		wamTeamGroupMap[i] = group;

		int alliance = GetPrivateProfileIntA(group, "AlliedGroup", 0, wpcstr);
		if(alliance < 0 || alliance > 5) alliance = 1;
		*(char*)(teamMeta + 0x1) = alliance;

		int vital = GetPrivateProfileIntA(group, "MissionVital", 0, wpcstr);
		*(char*)(teamMeta + 0x9A) = vital;

		int optional = GetPrivateProfileIntA(group, "Optional", 0, wpcstr);
		if(optional && *(char*)(teamMeta + 0x98) == 0) {
			numworms = 0;
		}

		if(*(char*)(teamMeta + 0x98) == 0) {
			// if empty team, set owner to CPU
			char skill = GetPrivateProfileIntA(group, "TeamSkill", 50, wpcstr);
			if(skill <= 0 || skill > 127) skill = 127;
			*(char*)(teamMeta) = skill * -1; // owner

			int cpulevel = ceil((float)skill / 20.0);
			char teamname[17];
			GetPrivateProfileStringA(group, "TeamNameValue", "", (LPSTR)&teamname, sizeof(teamname), wpcstr);
			if(strlen(teamname)) {
				strncpy((char*)(teamMeta + 0x3), teamname, 16);
			} else {
				int teamnameid = GetPrivateProfileIntA(group, "TeamNameNumber", 0, wpcstr);
				if(teamNames.find(teamnameid) == teamNames.end()) {
					numPlaceholders++;
					snprintf((char *) (teamMeta + 0x3), 10, "CPU Team %d", numPlaceholders, cpulevel);
				} else {
					strncpy((char*)(teamMeta + 0x3), teamNames.at(teamnameid).c_str(), 16);
				}
			}

			for (int a = 0; a < 8; a++) {
				char namefield[32];
				_snprintf_s(namefield, _TRUNCATE, "Worm%d_NameValue", a + 1);
				char wormname[17];
				GetPrivateProfileStringA(group, namefield, "", (LPSTR)&wormname, sizeof(wormname), wpcstr);

				_snprintf_s(namefield, _TRUNCATE, "Worm%d_NameNumber", a + 1);
				if(strlen(wormname)) {
					strncpy((char*)(teamMeta + 0x9B + 0x11 * a), wormname, 16);
				} else {
					int wormnameid = GetPrivateProfileIntA(group, namefield, 0, wpcstr);
					if (wormNames.find(wormnameid) == wormNames.end()) {
						snprintf((char *) (teamMeta + 0x9B + 0x11 * a), 6, "Worm %d", a + 1);
					} else {
						strncpy((char *) (teamMeta + 0x9B + 0x11 * a), wormNames.at(wormnameid).c_str(), 16);
					}
				}
			}

			if(cpuflags.find(cpulevel) != cpuflags.end()) {
				memcpy((void*)(teamMeta + 0x128), cpuflags[cpulevel], 0x554);
			}
		}
		if(numworms && *(char*)(teamMeta) < 0) hasCpuTeam = true;
		*(char*)(teamMeta + 0x98) = (char)numworms; // number of worms
		*(char*)(teamMeta + 0x124) = numworms != 0; // flag to copy team data
		count += numworms != 0;
	}
	*addrNumberOfTeams = count;
}

void Missions::setTeamAmmo(int team, int weapon, short int ammo, short int delay) {
	debugf("team: %d weapon: %d ammo: %d delay: %d\n", team, weapon, ammo, delay);
	*(short int*)(addrAmmoTable + 2 * (weapon + 143 * team)) = ammo;
	*(short int*)(addrAmmoTable + 2 * (weapon + 143 * team) + 142) = delay;
}

void Missions::readWamFile(fs::path wam) {
	if(fs::exists(wam)) {
		auto contents = Utils::readFile(wam.string());
		if(contents) {
			wamContents = std::move(*contents);
			static const int magiclen = 8;
			wamFlag = true;
			if(wamContents.length() > magiclen) {
				for(int i=0; i < magiclen; i++) {
					unsigned char c = wamContents[i];
					if(!std::isprint(c) && !std::isspace(c)) {
						Frontend::callMessageBox("Read a .WAM file with binary characters in header", 0, 0);
						resetCurrentWam();
						return;
					}
				}
			}
			wamDescription = describeCurrentWam(true);
			flagDisplayedDescription = false;
			wamTeamGroupMap.clear();
			wamAttemptNumber = 0;
			Scheme::setMissionWscScheme(wam);
			return;
		}
	}
	resetCurrentWam();
}

void Missions::resetCurrentWam() {
	if(wamFlag) {
		wamFlag = false;
		wamContents.clear();
		wamPath.clear();
		wamDescription.clear();
		wamTeamGroupMap.clear();
		wamAttemptNumber = 0;
		flagDisplayedDescription = false;
		Scheme::callSetBuiltinScheme(2);
		if(Packets::isHost()) {
			Protocol::sendResetWamToAllPlayers();
		}
		LobbyChat::lobbyPrint((char*)"WAM mission reset");
	}
}

void Missions::setWamContents(std::string wam) {
	wamContents = std::move(wam);
	wamFlag = true;
}

DWORD (__fastcall *origSelectFileInComboBox)(DWORD This, int EDX, char **a2);
DWORD __fastcall Missions::hookSelectFileInComboBox(DWORD This, int EDX, char **a2) {
	char * cwd = *(char**)(This + 0x424);
	char * basedir = *(char**)(This + 0x444);

	if(!strcmp(basedir, "user\\savedlevels\\")) {
		fs::path img(std::format("{}{}{}", basedir, cwd, *a2));
		fs::path wam = img.parent_path() / (img.stem().string() + ".wam");
		readWamFile(wam);
	}
//	else if(!strcmp(basedir, "User\\Schemes\\")) {
//	}
	return origSelectFileInComboBox(This, EDX, a2);
}

void Missions::onCreateReplay(nlohmann::json & config) {
	if(wamFlag) {
		config["wamFlag"] = wamFlag;
		auto ret = Scheme::saveSchemeToBytestr();
		if(ret) {
			auto & scheme = *ret;
			config["wamScheme"] = macaron::Base64::Encode(scheme);
			debugf("Saved scheme settings in replay json\n");
		}
	}
}

void Missions::onLoadReplay(nlohmann::json & config) {
	if(config.contains("wamFlag")) {
		wamFlag = config["wamFlag"];
		if(config.contains("wamScheme")) {
			macaron::Base64::Decode(config["wamScheme"], wamReplayScheme);
			debugf("Read scheme settings from replay json. Size: 0x%X\n", wamReplayScheme.size());
		}
	}
}

bool (__stdcall *origCheckGameEndCondition)();
bool __stdcall Missions::hookCheckGameEndCondition() {
	DWORD sesi;
	int retv;
	_asm mov sesi, esi

	DWORD gameglobal = *(DWORD*)(sesi + 0x2C);
	DWORD ddmain = *(DWORD*)(gameglobal + 0x24);
	int * condition = (int*)(ddmain + 0xD9D4);
	if(wamFlag && *condition == 2 && !hasCpuTeam) {
//		debugf("changing game end condition\n");
		*condition = 1;
	}

	_asm mov esi, sesi
	_asm call origCheckGameEndCondition
	_asm mov retv, eax

	return retv;
}

void Missions::onFrontendExit() {
	resetCurrentWam();
}

void Missions::onMapReset() {
	if(!Packets::isClient())
		resetCurrentWam();
}

void Missions::onExitMapEditor() {
	if(wamFlag && !flagDisplayedDescription) {
		LobbyChat::lobbyPrint((char*)"Playing with WAM file!");
		if(Packets::isHost()) {
			LobbyChat::lobbyPrint((char*)("\n" + wamDescription).c_str());
		} else {
			auto msg = describeCurrentWam(false);
			Frontend::callMessageBox(msg.c_str(), 0, 0);
		}
		flagDisplayedDescription = true;
	}
}

void Missions::onDestroyGameGlobal() {
	if(wamFlag) {
		*addrNumberOfTeams = prevNumberOfTeams;
	}
}

int (__stdcall *origCheckNumberOfRoundsToWinMatch)();
int __stdcall Missions::hookCheckNumberOfRoundsToWinMatch() {
	int a1, retv;
	_asm mov a1, eax

	if(wamFlag) return 1;
	_asm mov eax, a1
	_asm call origCheckNumberOfRoundsToWinMatch
	_asm mov retv, eax

	return retv;
}

int __stdcall Missions::hookReadWamEvents_patch1_c(char ** result, int id, char * group) {
	char buff[127];
	GetPrivateProfileStringA(group, "Text_String_Value", "", (LPSTR)&buff, sizeof(buff), wamPath.string().c_str());
	if(strlen(buff)) {
		WaLibc::CStringFromString(result, 0, buff, strlen(buff));
		return 1;
	}
	return 0;
}

DWORD addrReadWamEvents_patch1_ret_normal;
DWORD addrReadWamEvents_patch1_ret_hooked;
void __declspec(naked) Missions::hookReadWamEvents_patch1() {
//	_asm lea ecx, [esp+0x18]	// ptr to buffer
//	_asm push ecx	// ptr to buffer
//	_asm mov edx, eax // string index
	_asm lea ecx, [esp+0x18]
	_asm push eax // save string index

	_asm push ebp
	_asm push eax // index
	_asm push ecx // buffer
	_asm call hookReadWamEvents_patch1_c
	_asm test eax,eax
	_asm jnz hooked

_asm normal:
	_asm pop eax // restore index
	_asm lea ecx, [esp+0x18]
	_asm push ecx
	_asm mov edx, eax
	_asm jmp addrReadWamEvents_patch1_ret_normal
_asm hooked:
	_asm pop eax // restore index
	_asm jmp addrReadWamEvents_patch1_ret_hooked
}

void Missions::install() {
//	DWORD addrLoadWAMFile = ScanPatternf("LoadWAMFile", "\x6A\xFF\x64\xA1\x00\x00\x00\x00\x68\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x81\xEC\x00\x00\x00\x00\x53\x55\x8B\xAC\x24\x00\x00\x00\x00\x56\x57\xE8\x00\x00\x00\x00\x33\xC9\x85\xC0\x0F\x95\xC1\x85\xC9\x75\x0A", "????????x????xxxx????xx????xxxxx????xxx????xxxxxxxxxxx");
	DWORD addrFrontendLaunchMission = _ScanPattern("FrontendLaunchMission", "\x55\x8B\xEC\x83\xE4\xF8\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x51\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x53\x55\x56\x57\x8B\xE9\x68\x00\x00\x00\x00\xC7\x05\x00\x00\x00\x00\x00\x00\x00\x00", "??????xxx????xx????xxxx????xx????x????xxxxxxx????xx????????");
	DWORD addrLoadWAMFile = addrFrontendLaunchMission + 0x40 + *(DWORD*)(addrFrontendLaunchMission + 0x3C);
	DWORD addrWAMLoaderParams = *(DWORD*)(addrFrontendLaunchMission + 0x2D);
//	DWORD addrFrontendLaunchDeathmatch = ScanPatternf("FrontendLaunchDeathmatch", "\x55\x8B\xEC\x83\xE4\xF8\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x51\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x53\x55\x56\x57\x8B\xE9\x6A\x04\x8D\x4C\x24\x14", "??????xxx????xx????xxxx????xx????x????xxxxxxxxxxxx");
	DWORD addrCopyTeamsSchemeAndCreateReplay = _ScanPattern("CopyTeamsSchemeAndCreateReplay", "\x51\x53\x8B\x5C\x24\x10\x55\x8B\x6C\x24\x10\x83\xBD\x00\x00\x00\x00\x00\x56\x57\x75\x0A\xC7\x85\x00\x00\x00\x00\x00\x00\x00\x00\xC7\x45\x00\x00\x00\x00\x00\xB9\x00\x00\x00\x00", "??????xxxxxxx?????xxxxxx????????xx?????x????");
	DWORD addrCopyTeams = _ScanPattern("CopyTeams", "\x55\x8B\xEC\x83\xE4\xF8\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x81\xEC\x00\x00\x00\x00\x53\x56\x57\x33\xDB\x83\x3D\x00\x00\x00\x00\x00\x6A\x34", "??????xxx????xx????xxxx????xx????xxxxxxx?????xx");
	DWORD addrReadAmmoFromWAM = _ScanPattern("ReadAmmoFromWAM", "\x8B\x40\x0C\x56\x8B\x74\x24\x10\x57\x8B\x7C\x24\x0C\x50\x6A\x00\x51\x57\xFF\x15\x00\x00\x00\x00\x69\xF6\x00\x00\x00\x00\x03\x74\x24\x10\x0F\xB7\x14\x75\x00\x00\x00\x00\x66\x83\xFA\xFF", "??????xxxxxxxxxxxxxx????xx????xxxxxxxx????xxxx");
	DWORD addrSelectFileInComboBox = _ScanPattern("SelectFileInComboBox", "\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x83\xEC\x24\x53\x8B\x5C\x24\x38\x8B\x03\x8B\x40\xF4\x85\xC0\x55\x56\x57\x8B\xF1\xC6\x44\x24\x00\x00", "???????xx????xxxx????xxxxxxxxxxxxxxxxxxxxxxx??");
	DWORD addrCheckGameEndCondition = _ScanPattern("CheckGameEndCondition", "\x8B\x46\x2C\x83\xB8\x00\x00\x00\x00\x00\x74\x06\xB8\x00\x00\x00\x00\xC3\x8B\xC8\x8B\x49\x24\x80\xB9\x00\x00\x00\x00\x00\x74\x12", "??????????xxx????xxxxxxxx?????xx");
	DWORD addrCheckNumberOfRoundsToWinMatch = _ScanPattern("CheckNumberOfRoundsToWinMatch", "\x57\x8B\xF8\xE8\x00\x00\x00\x00\x85\xC0\x7F\x05\x83\xC8\xFF\x5F\xC3\x8B\x47\x04\x83\xF8\x02\x74\xF3\x85\xC0\x74\xEF", "????????xxxxxxxxxxxxxxxxxxxxx");
	DWORD addrGetMissionTitle = _ScanPattern("GetMissionTitle", "\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x51\x53\x8B\x5C\x24\x18\x56\xC7\x44\x24\x00\x00\x00\x00\x00\x8B\xF1\xC7\x44\x24\x00\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x33\xC9", "???????xx????xxxx????xxxxxxxxxx?????xxxxx?????x????xx");
	DWORD addrGetMissionDescription = _ScanPattern("GetMissionDescription", "\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x83\xEC\x08\x55\x8B\x6C\x24\x1C\x56\xC7\x44\x24\x00\x00\x00\x00\x00\x8B\xF1\xC7\x44\x24\x00\x00\x00\x00\x00\xE8\x00\x00\x00\x00", "???????xx????xxxx????xxxxxxxxxxxx?????xxxxx?????x????");
	DWORD addrReadWamEvents = _ScanPattern("ReadWamEvents", "\x6A\xFF\x64\xA1\x00\x00\x00\x00\x68\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x83\xEC\x10\x53\x55\x56\x57\x8B\xF1\x33\xDB\x33\xED\xE8\x00\x00\x00\x00\x33\xC9", "????????x????xxxx????xxxxxxxxxxxxxx????xx");
//	addrLoadCPUFlag = ScanPatternf("LoadCPUFlag", "\x69\xC0\x00\x00\x00\x00\x81\xEC\x00\x00\x00\x00\x56\x8D\x34\x08\x0F\xBE\x86\x00\x00\x00\x00\xF7\xD8\x0F\x88\x00\x00\x00\x00\x8D\x48\x13\xB8\x00\x00\x00\x00\xF7\xE9", "??????xx????xxxxxxx????xxxx????xxxx????xx");

	addrLobbyTeamList = *(DWORD*)(addrCopyTeams + 0x1DB);
	addrNumberOfTeams = *(char**)(addrCopyTeams + 0x8F);
	addrAmmoTable = *(DWORD*)(addrReadAmmoFromWAM + 0x26);

//	origSetBuiltinScheme = (int (__stdcall *)(int,int))(addrGetSchemeSettingsFromWam + 0xF + *(DWORD*)(addrGetSchemeSettingsFromWam + 0xB));
	addrPlayerInfoStruct = addrLobbyTeamList - 13 * 120;
	addrMissionTitleMap = *(DWORD*)(addrGetMissionTitle + 0xCF);
	addrMissionDescriptionMap = *(DWORD*)(addrGetMissionDescription + 0x112);

	debugf("addrLoadWAMFile: 0x%X addrWAMLoaderParams: 0x%X\naddrLobbyTeamList: 0x%X addrNumberOfTeams: 0x%X\naddrAmmoTable: 0x%X addrPlayerInfoStruct: 0x%X addrMissionTitleMap: 0x%X addrMissionDescriptionMap: 0x%X\n",
		   addrLoadWAMFile, addrWAMLoaderParams, addrLobbyTeamList,addrNumberOfTeams, addrAmmoTable,  addrPlayerInfoStruct, addrMissionTitleMap, addrMissionDescriptionMap);
	_HookDefault(LoadWAMFile);
//	HookfDefault(FrontendLaunchDeathmatch);
	_HookDefault(CopyTeamsSchemeAndCreateReplay);
	_HookDefault(CopyTeams);
	_HookDefault(SelectFileInComboBox);
	_HookDefault(CheckGameEndCondition);
	_HookDefault(CheckNumberOfRoundsToWinMatch);

	DWORD addrReadWamEvents_patch1 = addrReadWamEvents + 0x642;
	addrReadWamEvents_patch1_ret_normal = addrReadWamEvents + 0x649;
	addrReadWamEvents_patch1_ret_hooked = addrReadWamEvents + 0x64E;
	debugf("addrReadWamEvents_patch1: 0x%X ret_normal: 0x%X ret_hooked: 0x%X\n", addrReadWamEvents_patch1, addrReadWamEvents_patch1_ret_normal, addrReadWamEvents_patch1_ret_hooked);
	_HookAsm(addrReadWamEvents_patch1, (DWORD)&hookReadWamEvents_patch1);

	unsigned char teamskill[] = {0x83, 0xF8, 0x7F};
	_PatchAsm(addrLoadWAMFile + 0x308, (unsigned char*)&teamskill, sizeof(teamskill));
}

const std::string &Missions::getWamContents() {
	return wamContents;
}

bool Missions::getWamFlag() {
	return wamFlag;
}


void Missions::createMissionDirs() {
	if(Config::isDontCreateMissionDirs()) {debugf("createMissionDirs skipped\n"); return; }
	auto dirs = {"Data\\Mission", "User\\SavedLevels\\Mission\\WA", "User\\SavedLevels\\Mission\\WWP", "User\\SavedLevels\\Mission\\Custom"};
	for (auto & dir : dirs) {
		auto path = Config::getWaDir() / dir;
		fs::create_directories(path);
	}
}
void Missions::convertMissionFiles() {
	if(Config::isDontConvertMissionFiles()) {debugf("convertMissionFiles skipped\n"); return; }
	copyMissionFilesAttempts++;
	if(copyMissionFilesAttempts <= 2) {
		std::string datastr = WaLibc::getWaDataPath(true);
		std::filesystem::path datadir(datastr);
		debugf("data directory: |%s|\n", datastr.c_str());
		if(!datadir.empty()) {
			Missions::convertMissionFiles(datadir / "Mission", Config::getWaDir() / "user/SavedLevels/Mission/WA");
			copyMissionFilesAttempts++;
		} else {
			debugf("Failed to get mission files from CD (attempt: %d)\n", copyMissionFilesAttempts);
		}
	}
}


void Missions::convertMissionFiles(fs::path indir, fs::path outdir) {
	std::string ext(".wam");
	for (auto &p : fs::recursive_directory_iterator(indir)) {
		if (p.path().extension() == ext) {
			std::string s(p.path().filename().string());
			std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
			for(auto & prefix : {"mission", "training"}) {
				if (s.starts_with(prefix)) {
					convertWAM(p.path(), outdir, prefix);
				}
			}
		}
	}
}

void Missions::convertWAM(fs::path inpath, fs::path outdir, std::string prefix) {
	fs::path outfile = (outdir / inpath.filename());
	outfile.make_preferred();
	if(!fs::exists(outfile)) {
		int levelid = GetPrivateProfileIntA("Description", "Name", 1337, inpath.string().c_str());
		std::string imgname = std::format("{}{}.img", prefix, levelid);
		fs::path imgpath = inpath.parent_path() / imgname;

		int nametextid = 0;
		int descrtextid =0;
		if(prefix == "mission" && levelid >= 0 && levelid <= 33){
			nametextid = ((int*)addrMissionTitleMap)[levelid];
			descrtextid = ((int*)addrMissionDescriptionMap)[levelid];
		}
		else if(prefix == "training" && levelid >=1 && levelid <=16){
			if(levelid <= 14) {
				nametextid = ((int*)addrMissionTitleMap)[levelid + 33];
				descrtextid = ((int*)addrMissionDescriptionMap)[levelid + 33];
			} else if (levelid == 15){
				nametextid = 728;
				descrtextid = ((int*)addrMissionDescriptionMap)[levelid + 33];
			}
		}
		if(fs::exists(imgpath)) {
			fs::copy(inpath, outfile);
			fs::permissions(outfile, fs::perms::owner_all | fs::perms::group_all, fs::perm_options::add);
			if(nametextid) {
				std::string name = Frontend::origGetTextById(nametextid);
//				printf("file: %s levelid: %d nametextid: %d name: %s outfile: %s\n", inpath.filename().string().c_str(), levelid, nametextid, name.c_str(), outfile.string().c_str());
				WritePrivateProfileStringA("Mission", "NameText", name.c_str(), outfile.string().c_str());
			}
			if(descrtextid) {
				std::string descr = Frontend::origGetTextById(descrtextid);
				std::replace( descr.begin(), descr.end(), '\n', ' ');
				std::replace( descr.begin(), descr.end(), '\r', ' ');
//				printf("file: %s levelid: %d descrtextid: %d descr: %s outfile: %s\n", inpath.filename().string().c_str(), levelid, descrtextid, descr.c_str(), outfile.string().c_str());
				WritePrivateProfileStringA("Mission", "DescriptionText", descr.c_str(), outfile.string().c_str());
			}
			fs::copy(imgpath, outdir / (inpath.filename().stem().string() + ".img"));
		}
	}
}

std::string Missions::describeCurrentWam(bool formatting) {
	if(wamContents.empty()) return "!!! empty wam contents!";
	saveWamToTmpFile();
	std::string wp = wamPath.string();
	const char * wpcstr = wp.c_str();

	int multiplayer = GetPrivateProfileIntA("Mission", "Multiplay", 0, wpcstr);
	int condition = GetPrivateProfileIntA("Mission", "WinningCondition", 0, wpcstr);
	const std::string & conditionstr = winningConditions.find(condition) != winningConditions.end() ? winningConditions.at(condition) : "???";

	std::stringstream ss;
	if(formatting) ss << "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - \n";
	ss << "Custom Mission details:\n";
	if(formatting) ss << "- - - - - - - - - - - - - - - - - - - - - -\n";

	char name[128];
	GetPrivateProfileStringA("Mission", "NameText", "", (LPSTR)&name, sizeof(name), wpcstr);
	if(strlen(name))
		ss << "Name: " << name << "\n";
	char descr[2048];
	GetPrivateProfileStringA("Mission", "DescriptionText", "", (LPSTR)&descr, sizeof(descr), wpcstr);
	if(strlen(descr))
		ss << "Description: " << descr << "\n";

	ss << "Winning condition: " << condition  << " (" << conditionstr << ")\n";
	ss << "Teams:\n";

	for(int i=0; i < 6; i++) {
		char group[32];
		_snprintf_s(group, _TRUNCATE, "CPUTeam%d", i);
		if(i == 0)
			strcpy_s(group, "HumanTeam");
		unsigned int num = GetPrivateProfileIntA(group, "NumberOfWorms", 0, wpcstr);
		if(multiplayer && !num){
			_snprintf_s(group, _TRUNCATE, "HumanTeam%d", i);
			num = GetPrivateProfileIntA(group, "NumberOfWorms", 0, wpcstr);
			if(!num) {
				// fallback for multiplayer mission files without NumberOfWorms, but with Worm1_Energy
				for(int a=1; a <=8; a++) {
					char worm[32];
					_snprintf_s(worm, _TRUNCATE, "Worm%d_Energy", a);
					if(!GetPrivateProfileIntA(group, worm, 0, wpcstr)) break;
					num++;
				}
			}
		}
		if(num > 8) num = 0;

		int alliance = GetPrivateProfileIntA(group, "AlliedGroup", 0, wpcstr);
		if(alliance < 0 || alliance > 5) alliance = 1;

		int vital = GetPrivateProfileIntA(group, "MissionVital", 0, wpcstr);
		int optional = GetPrivateProfileIntA(group, "Optional", 0, wpcstr);
		if(num) {
			const std::string & alliancestr = alliances.find(alliance) != alliances.end() ? alliances.at(alliance) : "???";
			ss << std::format("   Team {} ({} alliance) - {} {}", i + 1, alliancestr, num, num == 1 ? "worm" : "worms");
			if(vital) ss << " [Vital - cannot be controlled by players]";
			if(optional) ss << " [Optional]";
			ss << "\n";
		}
	}

	if(formatting) ss << "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ";
	clearWamTmpFile();
	return ss.str();
}

const std::string &Missions::getWamDescription() {
	return wamDescription;
}

int Missions::getWamAttemptNumber() {
	return wamAttemptNumber;
}

void Missions::setWamAttemptNumber(int wamAttemptNumber) {
	if(wamAttemptNumber != Missions::wamAttemptNumber) {
		LobbyChat::lobbyPrint((char*)("Set WAM attempt number to: " + std::to_string(wamAttemptNumber)).c_str());
	}
	Missions::wamAttemptNumber = wamAttemptNumber;
}

bool Missions::getFlagInjectingWam() {
	return flagInjectingWAM;
}
