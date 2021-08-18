
#ifndef WKTERRAINSYNC_REPLAY_H
#define WKTERRAINSYNC_REPLAY_H

#include <vector>
#include <json.hpp>

class Replay {
public:
	static const unsigned int replayMagic = 0x37218814;
	static const unsigned int maxTerrainChunkSize = 2 * 1024 * 1024;
private:
	struct ReplayOffsets {
		size_t fileSize = 0;
		size_t mapChunkOffset = 4;
		size_t mapChunkSize = 0;
		size_t settingsChunkOffset = 0;
		size_t settingsChunkSize = 0;
		size_t terrainChunkOffset = 0;
		size_t taskMessageStreamOffset = 0;
	};

	static ReplayOffsets extractReplayOffsets(char * replayName);
	static inline std::vector<void(__stdcall *)(const char *)> loadReplayCallbacks;
	static inline std::vector<const char*(__stdcall *)(const char *)> createReplayCallbacks;
	static inline std::vector<int(__stdcall *)()> hasDataToSaveCallbacks;

	static inline bool replayPlaybackFlag = false;
	static inline bool flagExitEarly = false;
	static inline __time64_t lastTime = 0;

	static int __stdcall hookLoadReplay(int a1, int a2);
public:
	static void loadReplay(char * replayName);
	static void loadInfo(nlohmann::json & json);
	static void install();

	static bool isReplayPlaybackFlag();
	static int __stdcall hookCreateReplay(int a1, const char* a2, __time64_t Time);

	static void setFlagExitEarly(bool flagExitEarly);

	static __time64_t getLastTime();
	static void registerLoadReplayCallback(void(__stdcall * callback)(const char * jsonstr));
	static void registerCreateReplayCallback(const char*(__stdcall * callback)(const char * jsonstr));
	static void registerHasDataToSaveCallback(int(__stdcall * callback)());
};


#endif //WKTERRAINSYNC_REPLAY_H
