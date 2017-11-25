#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>


#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <UrbanLabs/Sdk/Utils/Logger.h>
/**
 * @brief The XmlConfig class
 */
class XmlConfig {
public:
    typedef std::unordered_map<std::string, std::string> Map;
    typedef std::set<std::string> Set;
    typedef std::unordered_map<std::string, Set> AssocArray;
private:
    std::string fileName_;
    xmlDocPtr doc_;
    xmlNodePtr propertyRoot_;
private:
    bool parseFile(const std::string &fileName, const std::string &propertyName);
    bool setReference(xmlNodePtr cur, const std::string &propertyName);

    std::string readProp(xmlNodePtr node, const std::string &propertyName) const;
    bool hasProp(xmlNodePtr node, const std::string &propertyName) const;
    bool isValid() const;
    bool isProperty(xmlNodePtr node, const std::string &propertyName,const std::string &propertyType) const;
    std::string readNodeContent(xmlNodePtr node) const;
    bool hasTagName(xmlNodePtr node, const std::string &tagName) const;
    bool readPair(xmlNodePtr node, std::pair<std::string,std::string> &p) const;
    bool readAssocArrayEntry(xmlNodePtr node, std::pair<std::string,Set> &p) const;
    bool readTagValue(xmlNodePtr node, const std::string &tagName, std::string &value) const;
    void init(xmlNodePtr node);
public:
    XmlConfig();
    ~XmlConfig();
    void setPropertyRoot(xmlNodePtr node);
    bool init(const std::string &fileName, const std::string &propertyName);
    bool getString(const std::string &propertyName, std::string &value) const;
    bool getBoolean(const std::string &propertyName, bool &value) const;
    bool getMap(const std::string &propertyName, Map &map) const;
    bool getStringSet(const std::string &propertyName, Set &collection) const;
    bool getAssocArray(const std::string &propertyName, AssocArray &aArray) const;
    XmlConfig getObject(const std::string &propertyName) const;
    bool getObjectCount(const std::string &propertyName, int &count) const;
    bool getObjectAt(const std::string &propertyName, int pos, XmlConfig &conf) const;
    bool hasProperty(const std::string &propName, const std::string &type) const;
};
