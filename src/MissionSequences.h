

#ifndef WKTERRAINSYNC_MISSIONSEQUENCES_H
#define WKTERRAINSYNC_MISSIONSEQUENCES_H

#include <array>
#include <vector>
#include "json.hpp"

typedef unsigned long DWORD;
typedef unsigned short int WORD;

struct SequenceEntry {
	int type;
	unsigned char data[128];
};

struct SequenceExeuctionState {
	DWORD ct200;
	WORD dd4ABA;
	DWORD gg4624;
	DWORD ct204;
	DWORD ct208;

	void backup(DWORD CTurnGame);
	void restore(DWORD CTurnGame);
};

class MissionSequences {
private:
	static inline std::vector<SequenceEntry> redSequence, blueSequence;
	static inline std::array<SequenceEntry, 128> * sequenceList;
	static inline int * sequenceListEventCount;

	static inline SequenceExeuctionState redState, blueState;
	static inline bool firstExec = true;

	static void doCustomSequences(DWORD cTurnGame);
	static int __fastcall hookReadWamEvents(DWORD This);
	static int __stdcall hookExecuteSequences();

	static void replaceSequenceList(std::vector<SequenceEntry> & newSequence);
	static int __stdcall hookDeserializeEvent();
	static void hookDeserializeEvent_before(SequenceEntry * event, unsigned char * stream);
	static void hookDeserializeEvent_after(SequenceEntry * event, unsigned char * stream);


public:
	static void install();
	static void onDestructGameGlobal();
	static void onCreateReplay_before();
	static void onCreateReplay_after(nlohmann::json &);

	static void init();
};


#endif //WKTERRAINSYNC_MISSIONSEQUENCES_H
