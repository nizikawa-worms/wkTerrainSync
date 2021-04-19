
#ifndef WKTERRAINSYNC_REPLAY_H
#define WKTERRAINSYNC_REPLAY_H


class Replay {
public:
	static const unsigned int replayMagic = 0x37218814;
	static const unsigned int maxTerrainChunkSize = 0x1000;

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
	static int __stdcall hookCreateReplay(int a1, int a2, __time64_t Time);

public:
	static void loadTerrainInfo(char * replayName);
	static void install();
};


#endif //WKTERRAINSYNC_REPLAY_H
