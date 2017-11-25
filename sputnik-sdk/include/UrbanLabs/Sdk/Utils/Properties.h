#pragma once

#include <map>
#include <string>
#include <initializer_list>

class Properties {
private:
    std::map<std::string, std::string> props_;
public:
    Properties();
    Properties(const std::map<std::string, std::string> &props);
    Properties(std::initializer_list<typename std::map<std::string, std::string>::value_type> props);
    bool set(const std::string &key, const std::string &val);
    bool has(const std::string &key) const;
    std::string get(const std::string &key) const;
    void put(const std::string &key, const std::string &val);
    std::map<std::string, std::string> list() const;
};
