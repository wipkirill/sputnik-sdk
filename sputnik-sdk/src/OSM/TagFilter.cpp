#include <UrbanLabs/Sdk/OSM/TagFilter.h>

TagFilter::AssocArray TagFilter::fixedKeyValue_ = {{}};

/**
 * @brief readConfig
 * @param configFile
 * @return
 */
bool TagFilter::readConfig(const string &configFile) {
    string confRoot = "tagFilter";
    XmlConfig xConf;
    if(!xConf.init(configFile, confRoot)) {
        LOGG(Logger::ERROR) << "Failed to init tagconfig" << Logger::FLUSH;
        return false;
    }

    if(!xConf.getAssocArray("fixedKeyValue", TagFilter::fixedKeyValue_)) {
        LOGG(Logger::ERROR) << "Failed to read fixedKeyValue for tagconfig" << Logger::FLUSH;
        return false;
    }

    return true;
}
