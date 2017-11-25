#pragma once

#include <map>
#include <mutex>
#include <string>
#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/Utils/URL.h>
#include <UrbanLabs/Sdk/Utils/Properties.h>
#include <UrbanLabs/Sdk/Storage/Storage.h>

// CREATE TABLE config(id INT, tag NVARCHAR(256), value NVARCHAR(256));
// sqlite> .separator "|"
// sqlite> .import sputnik.config.text config

class ConfigManager {
private:
    std::mutex lock_;
    TagStorage storage_;
    std::map<std::string, Properties> cache_;
public:
    ~ConfigManager();
    bool clear(const std::string &table);
    bool open(const URL &url, const Properties &props);
    bool get(const std::string &service, Properties &props);
    bool close();
};
