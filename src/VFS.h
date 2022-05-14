
#ifndef WKTERRAINSYNC_VFS_H
#define WKTERRAINSYNC_VFS_H


#include <vector>
#include <string>

class VFS {
private:
	DWORD vtable;
	DWORD dword4;
	DWORD dword8;
	DWORD dwordC;
	DWORD dword10;
	DWORD dword14;
	DWORD dword18;
	DWORD dword1C;
	DWORD dword20;
	DWORD dword24;
	DWORD dword28;
	DWORD dword2C;
	DWORD dword30;
	DWORD dword34;
	DWORD dword38;
	DWORD dword3C;
	DWORD dword40;
	DWORD dword44;
	DWORD dword48;
	DWORD dword4C;
	DWORD dword50;
	DWORD dword54;
	DWORD dword58;
	DWORD dword5C;
	DWORD dword60;
	DWORD dword64;
	DWORD dword68;
	DWORD dword6C;
	DWORD dword70;
	DWORD dword74;
	DWORD dword78;
	DWORD dword7C;
	DWORD dword80;
	DWORD dword84;
	DWORD dword88;
	DWORD dword8C;
	DWORD dword90;
	DWORD dword94;
	DWORD dword98;
	DWORD dword9C;
	DWORD dwordA0;
	DWORD dwordA4;
	DWORD dwordA8;
	DWORD dwordAC;
	DWORD dwordB0;
	DWORD dwordB4;
	DWORD dwordB8;
	DWORD dwordBC;
	DWORD dwordC0;
	DWORD dwordC4;
	DWORD dwordC8;
	DWORD dwordCC;
	DWORD dwordD0;
	DWORD dwordD4;
	DWORD dwordD8;
	DWORD dwordDC;
	DWORD dwordE0;
	DWORD dwordE4;
	DWORD dwordE8;
	DWORD dwordEC;
	DWORD dwordF0;
	DWORD dwordF4;
	DWORD dwordF8;
	DWORD dwordFC;
	DWORD dword100;
	DWORD dword104;
	DWORD dword108;
	DWORD dword10C;
	DWORD dword110;
	DWORD dword114;
	DWORD dword118;
	DWORD dword11C;
	DWORD dword120;
	DWORD dword124;
	DWORD dword128;
	DWORD dword12C;
	DWORD dword130;
	DWORD dword134;
	DWORD dword138;
	DWORD dword13C;
	DWORD dword140;
	DWORD dword144;
	DWORD dword148;
	DWORD dword14C;
	DWORD dword150;
	DWORD dword154;
	DWORD dword158;
	DWORD dword15C;
	DWORD dword160;
	DWORD dword164;
	DWORD dword168;
	DWORD dword16C;
	DWORD dword170;
	DWORD dword174;
	DWORD dword178;
	DWORD dword17C;
	DWORD dword180;
	DWORD dword184;
	DWORD dword188;
	DWORD dword18C;
	DWORD dword190;
	DWORD dword194;
	FILE* file_dword198;
	DWORD dword19C;
	DWORD dword1A0;
	DWORD dword1A4;
	DWORD dword1A8;
	DWORD dword1AC;
	DWORD dword1B0;
	DWORD dword1B4;
	DWORD dword1B8;

	struct VFS_Handle {
		DWORD * vtable;
		DWORD file_id;
		VFS * vfs;
	};
	struct VFS_Entry {
		DWORD dword0;
		DWORD dword4;
		DWORD dword8;
		DWORD dwordC;
		DWORD dword10;
		DWORD dword14;
		DWORD dword18;
	};


	static inline DWORD PC_FileArchive_vtable = 0;
public:
	static void install();
	static int callCheckIfFileExistsInVFS(const char * filename, DWORD vfs);
	DWORD callReadFileFromVFS(char * filename, DWORD vfs);
	static int callOpenVFS(DWORD vfs);

	VFS(std::filesystem::path path);
	~VFS();
	int openVFS();
	bool checkIfFileExists(const char * filename);
	std::string readFile(const char * filename);

	static void *operator new(size_t size);
	void operator delete(void*);
};


#endif //WKTERRAINSYNC_VFS_H
