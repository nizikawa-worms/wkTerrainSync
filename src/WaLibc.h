
#ifndef WKTERRAINSYNC_WALIBC_H
#define WKTERRAINSYNC_WALIBC_H

typedef unsigned long       DWORD;
#include <string>

class WaLibc {
private:
	static inline char * addrDriveLetter = nullptr;
	static inline char * addrSteamFlag = nullptr;
public:
	static void * waMalloc(size_t size);
	static void waFree(void * ptr);
	static inline int (__fastcall *CStringFromString)(void * This, int EDX, void *Source, size_t DestinationSize);
	static std::string getWaDataPath(bool addWaPath=false);
	static int install();
};


#endif //WKTERRAINSYNC_WALIBC_H
