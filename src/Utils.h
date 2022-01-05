
#ifndef WKTERRAINSYNC_UTILS_H
#define WKTERRAINSYNC_UTILS_H

#include <string>
#include <vector>
#include <set>
#include <optional>
#include <filesystem>

class Utils {
public:
	static void hexDump(const char *desc, const void *addr, int len);
	static void tokenize(std::string &str, const char* delim, std::vector<std::string> &out);
	static void tokenizeSet(std::string &str, const char* delim, std::set<std::string> &out);
	static std::optional<std::string> readFile(std::filesystem::path path);
	static void stripNonAlphaNum(std::string & s);
	static std::string compress_string(const std::string& str);
	static std::string decompress_string(const std::string& str);
	static std::string compress_file(std::filesystem::path file);
	static void decompress_file(std::string b64data, std::filesystem::path file);
};


#endif //WKTERRAINSYNC_UTILS_H
