
#ifndef WKTERRAINSYNC_WALIBC_H
#define WKTERRAINSYNC_WALIBC_H


class WaLibc {
public:
	static void * waMalloc(size_t size);
	static void waFree(void * ptr);

	static int install();
};


#endif //WKTERRAINSYNC_WALIBC_H
