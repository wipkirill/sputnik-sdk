#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <UrbanLabs/Sdk/GraphCore/BoundingBox.h>
#include <UrbanLabs/Sdk/Storage/Storage.h>
#include <UrbanLabs/Sdk/OSM/TagFilter.h>
#include "ParserPlugin.h"

class TagPlugin : public Plugin {
private:
    typedef Vertex::VertexId VertexId;
private:
    FILE *tagFile_;
    int tagsWritten_;
    TagFilter tagFilter_;
    std::string tagFileName_;
    std::string outFileName_;
    std::set<std::string> tags_;
    std::vector<std::string> locales_;
public:
    TagPlugin(const std::string &inputFile);
    virtual void init();
    virtual void notifyNode(OSMNode* node);
    virtual void notifyWay(OSMWay* way);
    virtual void finalize();
    virtual void cleanUp();
    virtual void validate();

    virtual std::vector<std::string> getTableNamesToImport() const;
    virtual std::vector<std::string> getSqlToCreateTables() const;
    virtual std::vector<std::string> getOtherSqlCommands() const;
};
