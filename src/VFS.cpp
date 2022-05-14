
#include "Debugf.h"
#include "Hooks.h"
#include "VFS.h"
#include "WaLibc.h"
#include <format>

DWORD addrCheckIfFileExistsInVFS;
int VFS::callCheckIfFileExistsInVFS(const char * filename, DWORD vfs) {
	int reta;

	_asm push vfs
	_asm mov eax, filename
	_asm call addrCheckIfFileExistsInVFS
	_asm mov reta, eax

	return reta;
}

DWORD addrReadFileFromVFS;
DWORD VFS::callReadFileFromVFS(char * filename, DWORD vfs) {
	DWORD retv;
	_asm mov esi, vfs
	_asm push filename
	_asm call addrReadFileFromVFS
	_asm mov retv, eax
	return retv;
}

DWORD addrOpenVFS;
int VFS::callOpenVFS(DWORD vfs) {
	int retv;
	_asm mov eax, vfs
	_asm call addrOpenVFS
	_asm mov retv, eax
	return retv;
}

VFS::VFS(std::filesystem::path path) {
	FILE * pf = WaLibc::origWaFopen((char*)path.string().c_str(), (char*)"rb");
	if(!pf) throw std::runtime_error(std::format("Failed to open VFS file: {}", path.string()));
	this->vtable = PC_FileArchive_vtable;
	this->file_dword198 = pf;
	if(!openVFS()) throw std::runtime_error("Failed to open VFS, this is a bug.");
}

VFS::~VFS() {
	if(this->file_dword198) {
		WaLibc::origWaFclose(this->file_dword198);
		this->file_dword198 = nullptr;
	}
}

void *VFS::operator new(size_t size) {
	return WaLibc::waMalloc(size);
}

void VFS::operator delete(void * ptr) {
	if(!ptr) return;
	WaLibc::waFree(ptr);
}

int VFS::openVFS() {
	return callOpenVFS((DWORD)this);
}


std::string VFS::readFile(const char *filename) {
	VFS_Handle * handle = (VFS_Handle*) callReadFileFromVFS((char*)filename, (DWORD)this);
	if(!handle) return "";
	VFS_Entry * entry = (VFS_Entry*)((DWORD)this + 16 * handle->file_id);
	if(!entry) return "";

	size_t size = entry->dword14 - entry->dword18;
	char * data = (char*)malloc(size + 1);
	if(!data) return "";

	data[size] = 0;
	(*(int (__thiscall **)(VFS_Handle*))(*(DWORD *)handle + 8))(handle);
	size_t read = (*(size_t (__thiscall **)(VFS_Handle*, char *, size_t))(*(DWORD *)handle + 20))(handle, data, size);
	if(!read) return "";
	std::string str(data, read);
	free(data);
	return str;
}

bool VFS::checkIfFileExists(const char * filename) {
	return callCheckIfFileExistsInVFS(filename, (DWORD)this);
}



void VFS::install() {
	addrCheckIfFileExistsInVFS = _ScanPattern("CheckIfFileExistsInVFS", "\x53\x55\x8B\x6C\x24\x0C\x56\x57\x8B\xF0\x8D\x9B\x00\x00\x00\x00\x56\x68\x00\x00\x00\x00\x68\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x6A\x7C\x68\x00\x00\x00\x00\xE8\x00\x00\x00\x00", "??????xxxxxx????xx????x????x????xxx????x????");
	addrReadFileFromVFS = _ScanPattern("ReadFileFromVFS", "\x83\xBE\x00\x00\x00\x00\x00\x75\x05\x33\xC0\xC2\x04\x00\x8B\x44\x24\x04\x53\x56\xE8\x00\x00\x00\x00\x8B\xD8\x85\xDB\x75\x04\x5B\xC2\x04\x00", "???????xxxxxxxxxxxxxx????xxxxxxxxxx");
	addrOpenVFS = _ScanPattern("OpenVFS", "\x83\xEC\x0C\x56\x8B\xF0\xE8\x00\x00\x00\x00\x8B\x06\x8B\x10\x6A\x04\x8D\x4C\x24\x10\x51\x8B\xCE\xFF\xD2\x83\xF8\x04\x74\x07\x33\xC0\x5E\x83\xC4\x0C\xC3", "??????x????xxxxxxxxxxxxxxxxxxxxxxxxxxx");
	DWORD addrGenerateMapIntoBitmapImage = _ScanPattern("GenerateMapIntoBitmapImage", "\x6A\xFF\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x81\xEC\x00\x00\x00\x00\x53\x55\x56\x57\x33\xED\x68\x00\x00\x00\x00\x89\x6C\x24\x1C\xE8\x00\x00\x00\x00\x83\xC4\x04\x68\x00\x00\x00\x00", "???????xx????xxxx????xx????xxxxxxx????xxxxx????xxxx????");
	PC_FileArchive_vtable = *(DWORD*)(addrGenerateMapIntoBitmapImage + 0x51);
	debugf("PC_FileArchive_vtable: 0x%X\n", PC_FileArchive_vtable);
}






