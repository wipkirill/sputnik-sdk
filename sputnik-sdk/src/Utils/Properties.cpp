#include <initializer_list>
#include <UrbanLabs/Sdk/Utils/Logger.h>
#include <UrbanLabs/Sdk/Utils/Properties.h>

using std::map;
using std::string;
using std::initializer_list;

/**
 * @brief Properties::Properties
 */
Properties::Properties() {;}
/**
 * @brief Properties::Properties
 * @param props
 */
Properties::Properties(const map<string, string> &props) : props_(props) {;}
/**
 * @brief Properties::Properties
 * @param props
 */
Properties::Properties(initializer_list<typename map<string, string>::value_type> props) : props_(props) {;}
/**
 * @brief set
 * @param key
 * @param val
 * @return
 */
bool Properties::set(const std::string &key, const std::string &val) {
    if(props_.find(key) != props_.end()) {
        LOGG(Logger::WARNING) << "[PROPERTIES] overriding key: " << key << Logger::FLUSH;
    }
    props_[key] = val;
    return true;
}
/**
 * @brief Properties::get
 * @param key
 * @return
 */
string Properties::get(const string &key) const {
    if(props_.find(key) == props_.end()) {
        LOGG(Logger::WARNING) << "[PROPERTIES] Key " << key << " not found" << Logger::FLUSH;
        return "";
    }
    return props_.find(key)->second;
}
/**
 * @brief has
 * @param key
 * @return
 */
bool Properties::has(const string &key) const {
    return props_.find(key) != props_.end();
}
/**
 * @brief Properties::list
 * @return
 */
map<string, string> Properties::list() const {
    return props_;
}
