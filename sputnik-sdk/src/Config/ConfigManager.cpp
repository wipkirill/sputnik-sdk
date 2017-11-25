#include <utility>
#include <UrbanLabs/Sdk/Config/ConfigManager.h>
#include <UrbanLabs/Sdk/Storage/Storage.h>
#include <UrbanLabs/Sdk/Storage/SqlConsts.h>

using std::pair;
using std::vector;
using std::string;

/**
 * @brief ConfigManager::open
 * @param url
 * @param props
 */
bool ConfigManager::open(const URL &url, const Properties &props) {
    if(!storage_.open(url, props)) {
        return false;
    }
    return true;
}
/**
 *
 */
ConfigManager::~ConfigManager() {
    storage_.close();
}
/**
 * @brief ConfigManager::get
 * @return
 */
bool ConfigManager::get(const string &service, Properties &props) {
    lock_.lock();
    if(cache_.find(service) != cache_.end()) {
        props = cache_[service];
        lock_.unlock();
        return true;
    }
    lock_.unlock();

    ConditionContainer conds;
    conds.addFieldRelationships({make_tuple("tag", "=", "service"),
                                 make_tuple("value", "=", service)});

    vector<TagList> tags;
    int limit = numeric_limits<int>::max(), offset = 0;
    if(!storage_.simpleSearch(SqlConsts::CONFIG_TABLE,conds,offset,limit,tags)) {
        LOGG(Logger::ERROR) << "couldn't load configuration" << Logger::FLUSH;
        return false;
    }
    if(tags.size() == 1) {
        for(const KeyValuePair &tag : tags[0].getTags()) {
            props.set(tag.getKey(), tag.getValue());
        }
        lock_.lock();
        cache_[service] = props;
        lock_.unlock();
        return true;        
    } else if(tags.size() > 1) {
        LOGG(Logger::ERROR) << "too many configurations found" << Logger::FLUSH;
    } else if(tags.size() == 1) {
        LOGG(Logger::ERROR) << "no configurations found" << Logger::FLUSH;
    }
    return false;
}
/**
 * @brief ConfigManager::clear
 * @param table
 * @return
 */
bool ConfigManager::clear(const string &table) {
    cache_.erase(table);
    return true;
}
/**
 * @brief close
 * @return
 */
bool ConfigManager::close() {
    cache_.clear();
    storage_.close();
    return true;
}
