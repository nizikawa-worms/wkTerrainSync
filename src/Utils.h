
#ifndef WKTERRAINSYNC_UTILS_H
#define WKTERRAINSYNC_UTILS_H

#include <string>
#include <vector>
#include <set>

class Utils {
public:
	static void hexDump(char *desc, void *addr, int len);
	static void tokenize(std::string &str, char* delim, std::vector<std::string> &out);
	static void tokenizeSet(std::string &str, char* delim, std::set<std::string> &out);
};


#endif //WKTERRAINSYNC_UTILS_H
