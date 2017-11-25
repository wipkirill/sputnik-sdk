#pragma once

#include "ParserPlugin.h"
#include <UrbanLabs/Sdk/OSM/TagFilter.h>
#include <UrbanLabs/Sdk/Storage/KdTreeSql.h>
#include <UrbanLabs/Sdk/Storage/Storage.h>
#include <UrbanLabs/Sdk/Utils/MathUtils.h>

#include <google/dense_hash_map>

#include <memory>

/**
 * @brief The TagCompressor class
 */
class TagCompressor {
public:
    typedef google::sparse_hash_map<std::string,std::string,std::hash<std::string> > StringMap;
private:
    int currId_;
    StringMap strMap_;
    TagFilter tagFilter_;
public:
    TagCompressor();
    std::pair<std::string,std::string> addKeyValue(const std::string &key, const std::string &value);
    std::string addString(const std::string &str);
    StringMap getTags();
private:
    std::string getCurrId();
};

/**
 * @brief The SearchPlugin class
 */
class SearchPlugin : public Plugin {
public:
    typedef Vertex::VertexId VertexId;
    typedef google::sparse_hash_set<VertexId,std::hash<Vertex> > VertexSet;
    typedef google::sparse_hash_map<std::string,VertexId,std::hash<string> > InternalIdMap;
private:
    // file for storing vertex and edge info
    std::string outputFileName_;
    // file stores id->osm_id
    std::string idToOsmIdFileName_;
    // file stores tags
    std::string tagsFileName_;
    std::string tagsFixedFileName_;
    std::string tagsFulltextFileName_;
    std::string tagsHashFileName_;
    // stores group relationships
    std::string relFileName_;
    FILE *idToOsmIdFileDescr_;
    // tags file descriptor
    FILE *tagsFileDescr_;
    FILE *tagsFixedFileDescr_;
    FILE *tagsFulltextFileDescr_;
    FILE *tagsHashFileDescr_;
    // group relationships
    FILE *relFileDescr_;
    VertexId freeInternalId_;
    TagFilter tagFilter_;
    TagCompressor tagCompr_;
    VertexSet objects_;
    VertexToPointIndexSql vpIndex_;
    InternalIdMap intenalIdToOsmId_;
    // address decoder
    std::map<std::string, std::shared_ptr<KdTreeSql> > addrTree_;
    // kdtree for osm objects
    KdTreeSql kdTreeObjects_;
    std::string vertexPointFileName_;
    // tags counters
    size_t totalTagsWritten_;
    size_t totalRelTagsWritten_;
    size_t totalkdTreeObjectsWritten_;
    size_t totalkdTreeAddressWritten_;
    // space filling curve
    BalancedPeanoCurve curve_;
public:
    SearchPlugin(const std::string &outputFile);
    virtual ~SearchPlugin();
    virtual void init();
    virtual void notifyNode(OSMNode* node);
    virtual void notifyWay(OSMWay* way);
    virtual void notifyRelation(OSMRelation* rel);
    virtual void finalize();
    virtual void validate();
    virtual void cleanUp();
    void closeFiles();

    virtual std::vector<std::string> getTableNamesToImport() const;
    virtual std::vector<std::string> getSqlToCreateTables() const;
    virtual std::vector<std::string> getOtherSqlCommands() const;
    virtual std::vector<std::string> getFileNamesToRead() const;
private:
    template<typename T>
    int rankObject(T *obj);
    template<typename T, typename IdType>
    void serializeFullTextObject(const IdType id, const T *obj, const Point &pt);
    template<typename T>
    void searializeTagValue(FILE *file, const T id, const std::string &tag, const std::string &value);
    void serializeFixedTagValue(FILE *file, Vertex::VertexId id, const std::string &keyval);
    void serializeWayMember(FILE *file, const Vertex::VertexId id, const Vertex::VertexId mem, int order);
    Vertex::VertexId getInternalId(const std::string &type, const Vertex::VertexId id);
    std::string getOuterId(const std::string &type, const Vertex::VertexId id);
    Vertex::VertexId extractOuterId(const std::string &id);
};
