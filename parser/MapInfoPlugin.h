#pragma once

#include <memory>
#include "ParserPlugin.h"
#include <UrbanLabs/Sdk/Storage/Storage.h>

class MapInfoPlugin : public Plugin {
public:
    typedef Vertex::VertexId VertexId;
    typedef google::sparse_hash_set<VertexId, std::hash<Vertex> > VertexSet;
private:
    std::string mapInfoFileName_;
    std::string outputFileName_;
    FILE * mapInfoFileDescriptor_;
    std::string vertexPointFileName_;
    VertexToPointIndexSql vpIndex_;
    std::set<std::string> mapFeatures_;
    std::unordered_map<std::string, std::string> features_;
    std::string areaName_;
    std::vector<std::string> countryCodes_;
    std::string comment_;
private:
    bool insertKeyValue(int id, const KeyValuePair &kv, std::unique_ptr<PrepStmt> &insertStmt);
public:
    MapInfoPlugin(const std::string& outputFile, const std::set<std::string> &mapFeatures);
    virtual ~MapInfoPlugin(){}
    void init();
    void setAreaName(const std::string &areaName);
    void setCountryCodes(const std::string &countryCodes);
    void setComment(const std::string &comment);
    virtual void afterImport();
    virtual void cleanUp();
    virtual std::vector<std::string> getTableNamesToImport() const;
    virtual std::vector<std::string> getSqlToCreateTables() const;
};
