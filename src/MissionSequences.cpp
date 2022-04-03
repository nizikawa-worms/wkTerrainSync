#include "MissionSequences.h"
#include "Hooks.h"
#include "Debugf.h"
#include "W2App.h"
#include <Windows.h>

int (__fastcall *origReadWamEvents)(DWORD This);
int __fastcall MissionSequences::hookReadWamEvents(DWORD This) {
	int ret = origReadWamEvents(This);

	init();

	int count = 0;
	for(int i=0; i < 300; i++) {
		char section[16];
		sprintf_s(section, "Sequence%04d", i);
		int type = GetPrivateProfileIntA(section, "TypeOfEvent", 0, *(LPCSTR *)(This + 12));
		if(type) {
			int sequenceColor = GetPrivateProfileIntA(section, "SequenceList", 0, *(LPCSTR *)(This + 12));
			auto & sequenceVector = sequenceColor == 0 ? redSequence : blueSequence;
			SequenceEntry & entry = sequenceList->at(count);
			sequenceVector.push_back(entry);
			count++;
		}
	}

	memset(sequenceList->data(), 0, sequenceList->size());
	memcpy(sequenceList->data(), redSequence.data(), redSequence.size() * sizeof(SequenceEntry));
	*sequenceListEventCount = redSequence.size();

	return ret;
}

void MissionSequences::init() {
	DWORD ddmain = W2App::getAddrDdMain();
	sequenceList = (decltype(sequenceList)) (ddmain + 0x8EFC);
	sequenceListEventCount = (int*)(ddmain + 0x4AFA);
	redSequence.clear();
	blueSequence.clear();
	firstExec = true;
}

int (__stdcall *origExecuteSequences)();
int __stdcall MissionSequences::hookExecuteSequences() {
	DWORD cTurnGame;
	int retv;
	_asm mov cTurnGame, eax

	if(firstExec && !blueSequence.empty()) {
		blueState.backup(cTurnGame);
		firstExec = false;
	}

	_asm mov eax, cTurnGame
	_asm call origExecuteSequences
	_asm mov retv, eax

	if(!blueSequence.empty()) {
		doCustomSequences(cTurnGame);
	}

	return retv;
}

void SequenceExeuctionState::backup(DWORD CTurnGame) {
	DWORD ddmain = W2App::getAddrDdMain();
	DWORD gameglobal = W2App::getAddrGameGlobal();

	ct200 = *(DWORD*)(CTurnGame + 0x200);
	dd4ABA = *(WORD*)(ddmain + 0x4ABA);
	gg4624 = *(DWORD*)(gameglobal + 0x4624);
	ct204 = *(DWORD*)(CTurnGame + 0x204);
	ct208 = *(DWORD*)(CTurnGame + 0x208);
}

void SequenceExeuctionState::restore(DWORD CTurnGame) {
	DWORD ddmain = W2App::getAddrDdMain();
	DWORD gameglobal = W2App::getAddrGameGlobal();

	*(DWORD*)(CTurnGame + 0x200) = ct200;
	*(WORD*)(ddmain + 0x4ABA) = dd4ABA;
	*(DWORD*)(gameglobal + 0x4624) = gg4624;
	*(DWORD*)(CTurnGame + 0x204) = ct204;
	*(DWORD*)(CTurnGame + 0x208) = ct208;
}

void MissionSequences::replaceSequenceList(std::vector<SequenceEntry> &newSequence) {
	memcpy(sequenceList->data(), newSequence.data(), newSequence.size() * sizeof(SequenceEntry));
	*sequenceListEventCount = newSequence.size();
}


void MissionSequences::doCustomSequences(DWORD cTurnGame) {
	redState.backup(cTurnGame);
	blueState.restore(cTurnGame);

	replaceSequenceList(blueSequence);

	_asm mov eax, cTurnGame
	_asm call origExecuteSequences

	replaceSequenceList(redSequence);

	blueState.backup(cTurnGame);
	redState.restore(cTurnGame);
}



DWORD hookSerializeEvent_patch1_ret;
void __declspec(naked) hookSerializeEvent_patch1() {
	_asm mov eax, [edi]
	_asm and eax, 0x7F
	_asm add eax, 0xFFFFFFFF
	_asm cmp eax, 0x12
	_asm jmp hookSerializeEvent_patch1_ret
}

static int counter = -1;
bool last_color = false;
void MissionSequences::hookDeserializeEvent_before(SequenceEntry *event, unsigned char *stream) {
	if(sequenceList == nullptr) {init();}
	if(event == sequenceList->data()) {
		counter = 0;
	}
	if(counter >= 0) {
		DWORD v4 = *(DWORD *)(stream + 8);
		unsigned char * type = (unsigned char*)(v4 + *(DWORD *)stream);
		last_color = *type & 0x80;
		*type &= 0x7F;
	}
}

void MissionSequences::hookDeserializeEvent_after(SequenceEntry *event, unsigned char *stream) {
	if(counter >= 0) {
		auto & sequence = last_color ? blueSequence : redSequence;
		sequence.push_back(*event);
		counter++;
	}
	if(counter == *sequenceListEventCount) {
		counter = -1;
		replaceSequenceList(redSequence);
	}
}

int (__stdcall *origDeserializeEvent)();
int __stdcall MissionSequences::hookDeserializeEvent() {
	unsigned char * stream;
	SequenceEntry * event;
	int retv;
	_asm mov event, eax
	_asm mov stream, ecx

	hookDeserializeEvent_before(event, stream);

	_asm mov eax, event
	_asm mov ecx, stream
	_asm call origDeserializeEvent
	_asm mov retv, eax

	hookDeserializeEvent_after(event, stream);

	return retv;
}



void MissionSequences::install() {
	DWORD addrReadWamEvents = Hooks::getScanNameToAddr().at("ReadWamEvents");
	DWORD addrExecuteSequences = _ScanPattern("ExecuteSequences", "\x53\x56\x57\x8B\xF8\x8B\x47\x2C\x8B\x48\x24\x0F\xB7\x91\x00\x00\x00\x00\x33\xDB\x39\x97\x00\x00\x00\x00\x7D\x5C\x8B\x47\x2C", "??????xxxxxxxx????xxxx????xxxxx");
	DWORD addrSerializeEvent = _ScanPattern("SerializeEvent", "\x53\x56\x8B\xF1\x8B\x4E\x04\x57\x83\xC1\x01\x39\x4E\x0C\x8B\xF8\x8A\x1F\x73\x05\xE8\x00\x00\x00\x00\x8B\x46\x04", "??????xxxxxxxxxxxxxxx????xxx");
	DWORD addrDeserializeEvent = _ScanPattern("DeserializeEvent", "\x51\x56\x57\x8B\xF9\x8B\xF0\x8B\x47\x08\x8D\x48\x01\x3B\x4F\x04\x76\x17\x68\x00\x00\x00\x00\x8D\x44\x24\x0C\x50\xC7\x44\x24\x00\x00\x00\x00\x00\xE8\x00\x00\x00\x00", "??????xxxxxxxxxxxxx????xxxxxxxx?????x????");

	_HookDefault(ReadWamEvents);
	_HookDefault(ExecuteSequences);
	_HookDefault(DeserializeEvent);

	DWORD addrSerializeEvent_patch1 = addrSerializeEvent + 0x25;
	hookSerializeEvent_patch1_ret = addrSerializeEvent + 0x2D;
	Hooks::hookAsm(addrSerializeEvent_patch1, (DWORD)&hookSerializeEvent_patch1);
}

void MissionSequences::onDestructGameGlobal() {
	redSequence.clear();
	blueSequence.clear();
	firstExec = true;
}

void MissionSequences::onCreateReplay_before() {
	if(!redSequence.empty() && !blueSequence.empty()) {
		std::vector<SequenceEntry> events = redSequence;
		for (auto entry: blueSequence) { // copy
			entry.type |= 0x80;
			events.push_back(entry);
		}
		replaceSequenceList(events);
	}
}

void MissionSequences::onCreateReplay_after(nlohmann::json &) {
	if(!redSequence.empty() && !blueSequence.empty()) {
		replaceSequenceList(redSequence);
	}
}

