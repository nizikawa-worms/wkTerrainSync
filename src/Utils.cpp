#include "Utils.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <zlib/zlib.h>
#include <Base64.h>
/*
 * Copyright 2018 Dominik Liebler

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
void Utils::hexDump(const char *desc, const void *addr, int len) {
	int i;
	unsigned char buff[17];
	unsigned char *pc = (unsigned char*)addr;

	// Output description if given.
	if (desc != NULL)
		printf ("%s:\n", desc);

	// Process every byte in the data.
	for (i = 0; i < len; i++) {
		// Multiple of 16 means new line (with line offset).

		if ((i % 16) == 0) {
			// Just don't print ASCII for the zeroth line.
			if (i != 0)
				printf("  %s\n", buff);

			// Output the offset.
			printf("  %04x ", i);
		}

		// Now the hex code for the specific character.
		printf(" %02x", pc[i]);

		// And store a printable ASCII character for later.
		if ((pc[i] < 0x20) || (pc[i] > 0x7e)) {
			buff[i % 16] = '.';
		} else {
			buff[i % 16] = pc[i];
		}

		buff[(i % 16) + 1] = '\0';
	}

	// Pad out last line if not exactly 16 characters.
	while ((i % 16) != 0) {
		printf("   ");
		i++;
	}

	// And print the final ASCII bit.
	printf("  %s\n", buff);
}

void Utils::tokenize(std::string & str, const char* delim, std::vector<std::string> &out) {
	char *token = strtok(const_cast<char*>(str.c_str()), delim);
	while (token != nullptr) {
		auto temp = std::string(token);
		if(!temp.empty())
			out.emplace_back(temp);
		token = strtok(nullptr, delim);
	}
}

void Utils::tokenizeSet(std::string & str, const char* delim, std::set<std::string> &out) {
	char *token = strtok(const_cast<char*>(str.c_str()), delim);
	while (token != nullptr) {
		auto temp = std::string(token);
		if(!temp.empty())
			out.insert(temp);
		token = strtok(nullptr, delim);
	}
}

std::optional<std::string> Utils::readFile(std::filesystem::path path) {
	std::ifstream in(path, std::ios::binary);
	if (in.good()) {
		std::stringstream buffer;
		buffer << in.rdbuf();
		in.close();
		return {buffer.str()};
	}
	return {};
}

void Utils::stripNonAlphaNum(std::string & s) {
	s.erase(std::remove_if(s.begin(), s.end(), []( auto const& c ) -> bool { return !std::isalnum(c); } ), s.end());
}


// Copyright 2007 Timo Bingmann <tb@panthema.net>
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)
//
// Original link http://panthema.net/2007/0328-ZLibString.html
// https://gist.github.com/gomons/9d446024fbb7ccb6536ab984e29e154a

/** Compress a STL string using zlib with given compression level and return
  * the binary data. */
std::string Utils::compress_string(const std::string& str) {
	z_stream zs;                        // z_stream is zlib's control structure
	memset(&zs, 0, sizeof(zs));

	if (deflateInit(&zs, Z_BEST_SPEED) != Z_OK)
		throw(std::runtime_error("deflateInit failed while compressing."));

	zs.next_in = (Bytef*)str.data();
	zs.avail_in = str.size();           // set the z_stream's input

	int ret;
	char outbuffer[32768];
	std::string outstring;

	// retrieve the compressed bytes blockwise
	do {
		zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
		zs.avail_out = sizeof(outbuffer);

		ret = deflate(&zs, Z_FINISH);

		if (outstring.size() < zs.total_out) {
			// append the block to the output string
			outstring.append(outbuffer,
			                 zs.total_out - outstring.size());
		}
	} while (ret == Z_OK);

	deflateEnd(&zs);

	if (ret != Z_STREAM_END) {          // an error occurred that was not EOF
		std::ostringstream oss;
		oss << "Exception during zlib compression: (" << ret << ") " << zs.msg;
		throw(std::runtime_error(oss.str()));
	}

	return outstring;
}

/** Decompress an STL string using zlib and return the original data. */
std::string Utils::decompress_string(const std::string& str) {
	z_stream zs;                        // z_stream is zlib's control structure
	memset(&zs, 0, sizeof(zs));

	if (inflateInit(&zs) != Z_OK)
		throw(std::runtime_error("inflateInit failed while decompressing."));

	zs.next_in = (Bytef*)str.data();
	zs.avail_in = str.size();

	int ret;
	char outbuffer[32768];
	std::string outstring;

	// get the decompressed bytes blockwise using repeated calls to inflate
	do {
		zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
		zs.avail_out = sizeof(outbuffer);

		ret = inflate(&zs, 0);

		if (outstring.size() < zs.total_out) {
			outstring.append(outbuffer,
			                 zs.total_out - outstring.size());
		}

	} while (ret == Z_OK);

	inflateEnd(&zs);

	if (ret != Z_STREAM_END) {          // an error occurred that was not EOF
		std::ostringstream oss;
		oss << "Exception during zlib decompression: (" << ret << ") "
		    << zs.msg;
		throw(std::runtime_error(oss.str()));
	}

	return outstring;
}

std::string Utils::compress_file(std::filesystem::path file) {
	std::ifstream in(file, std::ios::binary);
	if (in.good()) {
		std::stringstream istream;
		istream << in.rdbuf();
		in.close();
		std::string raw = istream.str();
		return Utils::compress_string(raw);
	} else {
		throw std::runtime_error(std::format("Failed to open file: {}", file.string()));
	}
}

void Utils::decompress_file(std::string b64data, std::filesystem::path file) {
	std::string data;
	macaron::Base64::Decode(b64data, data);
	data = decompress_string(data);
	std::ofstream of(file, std::ios::binary);
	if(of.good()) {
		of << data;
		of.close();
	} else {
		throw std::runtime_error(std::format("Failed to open file: {}", file.string()));
	}
}
