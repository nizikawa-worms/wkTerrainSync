#ifndef WKTERRAINSYNC_HOOKS_H
#define WKTERRAINSYNC_HOOKS_H


#include <map>
#include <MinHook.h>
#include <string>

class Hooks {
private:
	static inline std::map<std::string, DWORD> hookNameToAddr;
	static inline std::map<DWORD, std::string> hookAddrToName;

	static inline std::map<std::string, DWORD> scanNameToAddr;
	static inline std::map<DWORD, std::string> scanAddrToName;
	static inline bool scanFoundNew = false;

	//Worms development tools by StepS
	template<typename VT>
	static BOOL __stdcall PatchMemVal(PVOID pAddr, VT newValue) { return PatchMemData(pAddr, sizeof(VT), &newValue, sizeof(VT)); }
	template<typename VT>
	static BOOL __stdcall PatchMemVal(ULONG_PTR pAddr, VT newValue) { return PatchMemData((PVOID) pAddr, sizeof(VT), &newValue, sizeof(VT)); }

	static BOOL PatchMemData(PVOID pAddr, size_t buf_len, PVOID pNewData, size_t data_len);
	static BOOL InsertJump(PVOID pDest, size_t dwPatchSize, PVOID pCallee, DWORD dwJumpType);
public:
	static MH_STATUS minhook(std::string name, DWORD pTarget, DWORD *pDetour, DWORD *ppOriginal);
	static void hookApi(std::string name, LPCWSTR pszModule, LPCSTR pszProcName, DWORD *pDetour, DWORD *ppOriginal);

	static void hookAsm(DWORD startAddr, DWORD hookAddr);
	static void hookVtable(const char * classname, int offset, DWORD addr, DWORD hookAddr, DWORD * original);
	static void patchAsm(DWORD addr, unsigned char *op, size_t opsize);

	static DWORD scanPattern(const char* name, const char* pattern, const char* mask, DWORD expected = 0);
};


#endif //WKTERRAINSYNC_HOOKS_H
