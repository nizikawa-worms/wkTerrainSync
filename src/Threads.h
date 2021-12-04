

#ifndef WKTERRAINSYNC_THREADS_H
#define WKTERRAINSYNC_THREADS_H

#include <thread>

class Threads {
private:
	static inline std::thread dataScanThread;
public:
	static void startDataScan();
	static void awaitDataScan();
};


#endif //WKTERRAINSYNC_THREADS_H
