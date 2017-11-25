#pragma once

#include <string>

/**
 * scheme://domain:port/path?query#fragment
 * @brief The URL class
 */
class URL {
private:
    int port;
    std::string scheme_;
    std::string domain_;
    std::string path_;
    std::string query_;
    std::string frag_;
public:
    URL(const std::string &url);
    int getPort() const;
    std::string getScheme() const;
    std::string getResource() const;
    std::string getPath() const;
    std::string getQuery() const;
    std::string getFragment() const;
};
