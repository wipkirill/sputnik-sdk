#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "ParserPlugin.h"

#include <UrbanLabs/Sdk/GraphCore/BoundingBox.h>
#include <UrbanLabs/Sdk/Storage/Storage.h>
#include <UrbanLabs/Sdk/OSM/TagFilter.h>

class AddressDecoderPlugin : public Plugin {
private:
    typedef Vertex::VertexId VertexId;
private:
    TagFilter tagFilter_;
    FILE *addrAreasFile_;
    std::string outputFileName_;
    std::string vertexPointFileName_;
    std::string areasOutFileName_;
    VertexToPointIndexSql vptIndex_;
    std::string translateDbName_;
    std::unordered_map<std::string, std::unordered_map<std::string, BoundingBox> > bbox_;
private:
    std::unique_ptr<DbConn> dbConn_;
    std::unique_ptr<PrepStmt> toLower_, toUpper_;
private:
    std::string toUpper(const std::string &str);
    std::string toLower(const std::string &str);
    bool validAddressTag(const std::string &type, const std::string &name);
public:
    AddressDecoderPlugin(const std::string &inputFile);
    virtual void init();
    virtual void notifyNode(OSMNode* node);
    virtual void notifyWay(OSMWay* way);
    virtual void finalize();
    virtual void cleanUp();

    virtual std::vector<std::string> getTableNamesToImport() const;
    virtual std::vector<std::string> getSqlToCreateTables() const;
    virtual std::vector<std::string> getOtherSqlCommands() const;
private:
    void addAddressArea(const std::string &type, const std::string &name, const Point &pt);
};
