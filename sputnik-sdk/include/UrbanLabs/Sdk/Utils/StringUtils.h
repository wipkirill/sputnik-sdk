#pragma once

#include <set>
#include <vector>
#include <algorithm>
#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/Utils/Types.h>

class StringConsts {
public:
    const static std::string SEPARATOR;
    const static std::string PT;
    const static std::string COMMA;
    const static std::string LINEBREAK;
    const static std::string WHITESPACE;
    const static int MAX_TAG_LENGTH = 50;
};
/**
 * @brief drawProgressBar
 * @param processed
 * @param total
 */
void drawProgressBar(size_t processed, size_t total);
/**
 * @brief getStdoutFromCommand
 * @param cmd
 * @return
 */
std::string getStdoutFromCommand(const std::string& cmd);

/**
 * @brief The SimpleTokenator class
 */
class SimpleTokenator {
    size_t pos_;
    std::string str_;
    std::vector<int> brk_;
    char delim_;
    char nonBreak_;
    bool omit_;
public:
    SimpleTokenator(const std::string &line, char delim, char nonBreak, bool omit);
    static bool isValid(char nonBreak, const std::string &str);
    std::vector <std::string> getTokens();
    std::string nextToken();
    size_t countTokens();
    bool isValid() const;
    void reset();
};

/**
 * @brief The StringUtils class
 */
class StringUtils {
public:
    static std::string trim(const std::string& s);
    static std::string substr(const std::string& s, int length);
    static std::string trimLeft(const std::string& s);
    static std::string trimRight(const std::string& s);
    static std::string erase(std::string &s,const std::string &toEraseChars);
    static bool isSpace(char c);
    static bool isNotspace(char c);
    static std::vector<std::string> split(const std::string& s);
    static std::vector<std::string> filterWords(const std::vector<std::string>& rawWords);
    static std::string quote(const std::string &s);
    static std::string escape(const std::string &s);
    static std::string replaceAll(std::string const& original,
                                  std::string const& before,
                                  std::string const& after);
    static std::string base64_encode(const char *str, size_t len);
    static std::string base64_decode(std::string const& s);
    static std::string base64_encode(int num);
    static std::string stringToHex(const char* data, size_t len);
    static std::string base36_encode(int num);
};
/**
 * @brief The Folder class
 */
class Folder {
public:
    static const std::string COMMON;
    static const std::string FONTS;
    static const std::string DATA;
    static const std::string PLUGIN_INPUT;
    static const std::string TILES;
    static const std::string RESOURCE;
    static const std::string IMG;
};
/**
 * @brief The FileName class
 */
class FileName {
public:
    static const std::string INDEX_HTML;
    static const std::string MENU_HTML;
    static const std::string ERROR_HTML;
    static const std::string MAPNIK_XML;
    static const std::string LABELS_XML;
};
