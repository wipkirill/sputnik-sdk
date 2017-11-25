#pragma once

#include <string>

class BzipUtil {
public:
    static bool isDecompressable(const std::string& infileName);
    static bool decompress(const std::string& infileName, const std::string& outFileName, bool removeSrc = false);
};
// class SnappyUtil {
// public:
// 	static void compress(const std::string &input, std::string &output);
// 	static void decompress(const std::string& input, std::string &output);
// };
