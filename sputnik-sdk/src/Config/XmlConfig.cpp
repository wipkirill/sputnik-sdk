#include <UrbanLabs/Sdk/Config/XmlConfig.h>

using namespace std;

/**
 * @brief XmlConfig::XmlConfig
 */
XmlConfig::XmlConfig():fileName_(),doc_(0),propertyRoot_(0) {
    ;
}
/**
 * @brief XmlConfig::init
 * @param node
 */
void XmlConfig::init(xmlNodePtr node) {
    propertyRoot_ = node;
}
/**
 * @brief XmlConfig::parseFile
 * @param fileName
 * @param propertyName
 * @return
 */
bool XmlConfig::parseFile(const string &fileName, const string &propertyName) {
    doc_ = xmlParseFile(fileName.c_str());

    if(doc_ == NULL) {
        LOGG(Logger::ERROR) << "Failed to parse xml in "<<fileName<< Logger::FLUSH;
        return false;
    }
    xmlNodePtr cur = xmlDocGetRootElement(doc_);
    if (cur == NULL) {
        LOGG(Logger::ERROR) << "Empty document"<<fileName<< Logger::FLUSH;
        xmlFreeDoc(doc_);
        return false;
    }

    if (xmlStrcmp(cur->name, (const xmlChar *) "configuration")) {
        LOGG(Logger::ERROR) <<"Document of the wrong type, root node != configuration"<< Logger::FLUSH;
        xmlFreeDoc(doc_);
        return false;
    }
    return setReference(cur, propertyName);
}
/**
 * @brief XmlConfig::setReference
 * @param cur
 * @param propertyName
 * @return
 */
bool XmlConfig::setReference(xmlNodePtr cur, const string &propertyName) {
    cur = cur->xmlChildrenNode;
    string name;
    bool ret = false;
    while (cur != NULL) {
        if (hasTagName(cur, "property")) {
            if(!hasProp(cur, "name")) {
                return false;
            }
            name = readProp(cur, "name");
            if(propertyName == name) {
                ret = true;
                propertyRoot_ = cur;
                break;
            }
        }
        cur = cur->next;
    }
    return ret;
}
/**
 * @brief readProp
 * @param node
 * @param propertyName
 * @return
 */
string XmlConfig::readProp(xmlNodePtr node, const string &propertyName) const {
    return string((const char*)xmlGetProp(node, (const xmlChar *)propertyName.c_str()));
}
/**
 * @brief hasProp
 * @param node
 * @param propertyName
 * @return
 */
bool XmlConfig::hasProp(xmlNodePtr node, const string &propertyName) const {
    if(!xmlHasProp(node, (const xmlChar *)propertyName.c_str())) {
        LOGG(Logger::WARNING) << "No property"<<propertyName<<"specified in xml"<< Logger::FLUSH;
        return false;
    }
    return true;
}
/**
 * @brief hasTagName
 * @param node
 * @param tagName
 * @return
 */
bool XmlConfig::hasTagName(xmlNodePtr node, const std::string &tagName) const {
    if (!xmlStrcmp(node->name, (const xmlChar *)tagName.c_str()))
        return true;
    return false;
}
/**
 * @brief isValid
 * @return
 */
bool XmlConfig::isValid() const {
    return propertyRoot_ != 0;
}
/**
 * @brief isProperty
 * @param node
 * @param propertyName
 * @param propertyType
 * @return
 */
bool XmlConfig::isProperty(xmlNodePtr node, const string &propertyName,const string &propertyType) const {
    if (hasTagName(node, "property")) {
        if(!hasProp(node, "name")) {
            return false;
        }
        string name = readProp(node, "name");
        if(!hasProp(node, "type")) {
            return false;
        }
        string type = readProp(node, "type");
        if(propertyName == name && type == propertyType) {
            return true;
        }
    }
    return false;
}
/**
 * @brief readNodeContent
 * @param node
 * @return
 */
string XmlConfig::readNodeContent(xmlNodePtr node) const {
    if(xmlNodeGetContent(node) != NULL)
        return string((const char*)xmlNodeGetContent(node));
    return "";
}
/**
 * @brief readPair
 * @param node
 * @param p
 * @return
 */
bool XmlConfig::readPair(xmlNodePtr node, pair<string,string> &p) const {
    if (hasTagName(node,"pair")) {
        if(!hasProp(node, "key")) {
            return false;
        }
        if(!hasProp(node, "value")) {
            return false;
        }
        p.first = readProp(node, "key");
        p.second = readProp(node, "value");
        if(p.first == "" || p.second =="")
           LOGG(Logger::WARNING) <<"Reading empty key or value in pair"<< Logger::FLUSH;
        return true;
    }
    return false;
}
/**
 * @brief XmlConfig::readAssocArrayEntry
 * @param node
 * @param p
 * @return
 */
bool XmlConfig::readAssocArrayEntry(xmlNodePtr node, std::pair<std::string,Set> &p) const {
    if (hasTagName(node,"entry")) {
        if(!hasProp(node, "key")) {
            return false;
        }
        if(readProp(node, "key") == "") {
            LOGG(Logger::WARNING) <<"Reading empty key in assoc array"<< Logger::FLUSH;
            return false;
        }
        p.first = readProp(node, "key");
        xmlNodePtr cur = node->xmlChildrenNode;
        while(cur != NULL) {
            string s;
            if(readTagValue(cur,"value", s))
                p.second.insert(s);
            cur = cur->next;
        }
        return true;
    }
    return false;
}
/**
 * @brief readValue
 * @param node
 * @param tagName
 * @param value
 * @return
 */
bool XmlConfig::readTagValue(xmlNodePtr node, const string& tagName, string &value) const {
    if (hasTagName(node, tagName)) {
        value = readNodeContent(node);
        if(value == "")
           LOGG(Logger::WARNING) <<"Reading empty value from node content"<< Logger::FLUSH;
        return true;
    }
    return false;
}
/**
 * @brief XmlConfig::~XmlConfig
 */
XmlConfig::~XmlConfig() {
    if(doc_ != 0) {
        LOGG(Logger::WARNING) <<"Destructing XML doc in config structure"<< Logger::FLUSH;
        xmlFreeDoc(doc_);
    } else
        LOGG(Logger::WARNING) <<"Destructing a child config structure"<< Logger::FLUSH;
}
/**
 * @brief init
 * @param fileName
 * @return
 */
bool XmlConfig::init(const string& fileName, const string &propertyName) {
    if(!parseFile(fileName, propertyName)) {
        return false;
    }
    return true;
}
/**
 * @brief getString
 * @param propertyName
 * @param value
 * @return
 */
bool XmlConfig::getString(const string &propertyName, string &value) const {
    if(!isValid()) {
        LOGG(Logger::ERROR) << "Not initialized config structure"<< Logger::FLUSH;
        return false;
    }
    bool ret = false;
    xmlNodePtr cur = propertyRoot_->xmlChildrenNode;
    while (cur != NULL) {
        if (isProperty(cur, propertyName, "string")) {
            value = readNodeContent(cur);
            ret = true;
            break;
        }
        cur = cur->next;
    }
    if(!ret) {
        LOGG(Logger::WARNING) << "Property"<<propertyName<<"of type string not found"<< Logger::FLUSH;
    }
    return ret;
}
/**
 * @brief getMap
 * @param propertyName
 * @param map
 * @return
 */
bool XmlConfig::getMap(const string &propertyName, Map &map) const {
    if(!isValid()) {
        LOGG(Logger::ERROR) << "Not initialized config structure"<< Logger::FLUSH;
        return false;
    }
    bool ret = false;
    xmlNodePtr cur = propertyRoot_->xmlChildrenNode;
    while (cur != NULL) {
        if (isProperty(cur, propertyName, "map")) {
            cur = cur->xmlChildrenNode;
            while(cur != NULL) {
                pair<string,string> p;
                if(readPair(cur, p))
                    map.insert(p);
                cur = cur->next;
            }
            ret = true;
            break;
        }
        cur = cur->next;
    }
    if(!ret) {
        LOGG(Logger::WARNING) << "Property"<<propertyName<<"of type map not found"<< Logger::FLUSH;
    }
    return ret;
}
/**
 * @brief getArray
 * @param propertyName
 * @param collection
 * @return
 */
bool XmlConfig::getStringSet(const string &propertyName, Set &collection) const {
    if(!isValid()) {
        LOGG(Logger::ERROR) << "Not initialized config structure"<< Logger::FLUSH;
        return false;
    }
    bool ret = false;
    xmlNodePtr cur = propertyRoot_->xmlChildrenNode;
    while (cur != NULL) {
        if (isProperty(cur, propertyName, "array")) {
            cur = cur->xmlChildrenNode;
            while(cur != NULL) {
                string s;
                if(readTagValue(cur,"value", s))
                    collection.insert(s);
                cur = cur->next;
            }
            ret = true;
            break;
        }
        cur = cur->next;
    }
    if(!ret) {
        LOGG(Logger::WARNING) << "Property"<<propertyName<<"of type array not found"<< Logger::FLUSH;
    }
    return ret;
}
/**
 * @brief getObject
 * @param propertyName
 * @return
 */
XmlConfig XmlConfig::getObject(const string &/*propertyName*/) const {
    return XmlConfig();
}
/**
 * @brief XmlConfig::getObjectCount
 * @param propertyName
 * @param count
 * @return
 */
bool XmlConfig::getObjectCount(const string &propertyName, int &count) const {
    if(!isValid()) {
        LOGG(Logger::ERROR) << "Not initialized config structure"<< Logger::FLUSH;
        return false;
    }
    bool ret = false;
    xmlNodePtr cur = propertyRoot_->xmlChildrenNode;
    while (cur != NULL) {
        if (isProperty(cur, propertyName, "objects")) {
            cur = cur->xmlChildrenNode;
            while(cur != NULL) {
                if(hasTagName(cur, "object"))
                    count++;
                cur = cur->next;
            }
            ret = true;
            break;
        }
        cur = cur->next;
    }
    if(!ret) {
        LOGG(Logger::WARNING) << "Property"<<propertyName<<"not found"<< Logger::FLUSH;
    }
    return ret;
}
/**
 * @brief XmlConfig::getBoolean
 * @param propertyName
 * @param value
 * @return
 */
bool XmlConfig::getBoolean(const std::string &propertyName, bool &value) const {
    if(!isValid()) {
        LOGG(Logger::ERROR) << "Not initialized config structure"<< Logger::FLUSH;
        return false;
    }
    bool ret = false;
    xmlNodePtr cur = propertyRoot_->xmlChildrenNode;
    while (cur != NULL) {
        if (isProperty(cur, propertyName, "boolean")) {
            string val = readNodeContent(cur);
            if(val == "true") {
                ret = true;
                value = true;
            } else if(val == "false") {
                value = false;
                ret = true;
            } else {
                ret = false;
            }
            break;
        }
        cur = cur->next;
    }
    if(!ret) {
        LOGG(Logger::WARNING) << "Property"<<propertyName<<"of type boolean not found"<< Logger::FLUSH;
    }
    return ret;
}
/**
 * @brief getObjectAt
 * @param propertyName
 * @param pos
 * @return
 */
bool XmlConfig::getObjectAt(const std::string &propertyName, int pos, XmlConfig &conf) const {
    if(!isValid()) {
        LOGG(Logger::ERROR) << "Not initialized config structure"<< Logger::FLUSH;
        return false;
    }
    bool ret = false;
    xmlNodePtr cur = propertyRoot_->xmlChildrenNode;
    while (cur != NULL) {
        if (isProperty(cur, propertyName, "objects")) {
            xmlNodePtr obj = cur->xmlChildrenNode;
            int count = 0;
            while(obj != NULL) {
                if(hasTagName(obj, "object")) {
                    if(pos == count) {
                        conf.init(obj);
                        ret = true;
                        return ret;
                    }
                    count++;
                }
                obj = obj->next;
            }
            break;
        }
        cur = cur->next;
    }
    if(!ret) {
        LOGG(Logger::WARNING) << "Property"<<propertyName<<"not found"<< Logger::FLUSH;
    }
    return ret;
}
/**
 * @brief XmlConfig::getAssocArray
 * @param propertyName
 * @param aArray
 * @return
 */
bool XmlConfig::getAssocArray(const std::string &propertyName, AssocArray &aArray) const {
    if(!isValid()) {
        LOGG(Logger::ERROR) << "Not initialized config structure"<< Logger::FLUSH;
        return false;
    }
    bool ret = false;
    xmlNodePtr cur = propertyRoot_->xmlChildrenNode;
    while (cur != NULL) {
        if (isProperty(cur, propertyName, "associativeArray")) {
            cur = cur->xmlChildrenNode;
            while(cur != NULL) {
                pair<string,Set> p;
                if(readAssocArrayEntry(cur, p))
                    aArray.insert(p);
                cur = cur->next;
            }
            ret = true;
            break;
        }
        cur = cur->next;
    }
    if(!ret) {
        LOGG(Logger::WARNING) << "Property"<<propertyName<<"of type associativeArray not found"<< Logger::FLUSH;
    }
    return ret;
}
/**
 * @brief XmlConfig::hasProperty
 * @param propName
 * @return
 */
bool XmlConfig::hasProperty(const string &propName, const string &type) const {
    if(!isValid()) {
        LOGG(Logger::ERROR) << "Not initialized config structure"<< Logger::FLUSH;
        return false;
    }
    bool ret = false;
    xmlNodePtr cur = propertyRoot_->xmlChildrenNode;
    while (cur != NULL) {
        if (isProperty(cur, propName, type)) {
            ret = true;
            break;
        }
        cur = cur->next;
    }
    if(!ret) {
        LOGG(Logger::WARNING) << "Property"<<propName<<"of type"<<type<<"not found"<< Logger::FLUSH;
    }
    return ret;
}
