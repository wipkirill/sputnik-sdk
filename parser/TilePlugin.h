#pragma once

#include "ParserPlugin.h"
#include "OGR/OGR.h"
#include "OGR/OGRTypes.h"
#include "OGR/Polygon.h"
#include "OGR/LineString.h"

#include <UrbanLabs/Sdk/Utils/StringUtils.h>
#include <UrbanLabs/Sdk/Storage/Storage.h>
#include <UrbanLabs/Sdk/Utils/GPolyEncode.h>
#include <UrbanLabs/Sdk/Config/XmlConfig.h>

#include <google/sparse_hash_set>

#include <set>
#include <unordered_map>
#include <unordered_set>
#include <iostream>

class TileLayer {
public:
    typedef Vertex::VertexId VertexId;
    typedef google::sparse_hash_map<VertexId, std::vector<VertexId>, std::hash<VertexId> > WayNodesMap;
    typedef std::unordered_map<std::string, std::set<std::string> > AssocArray;
    typedef std::set<std::string> StringSet;
    typedef std::unordered_map<std::string,std::string> StyleGroupMap;
    typedef std::unordered_map<std::string,std::string> ComputedField;
    typedef std::map<std::string, ComputedField> ComputedFieldMap;
public:
    enum GeometryType { POINT, POLYGON, LINE };
public:
    // determines which lines can be split
    static StringSet nonSplittable_;
    static StyleGroupMap styleGroup_;
protected:
    // layer id(table name)
    std::string name_;
    FILE * layerDesc_;
    // layer type
    GeometryType type_;
    // create stmt
    std::string tableStmt_;
    std::string baseFileName_;
    // create table with these fields additionally
    StringSet tableFields_;
    // computed fields such as stylegroup and area
    ComputedFieldMap computedFields_;
    // Vertex Point index
    VertexToPointIndexSql vpIndex_;
    // determines which keys and values are accepted
    AssocArray kv_;
    bool compression_;
public:
    // used to assign ids in the table
    static size_t totalPoints_;
    static size_t totalLines_;
    static size_t totalPolygons_;
public:
    TileLayer(const std::string &baseFileName, const std::string &name, GeometryType lType,
              const AssocArray &kv, const StringSet &tableFields,
              const ComputedFieldMap &computedFields, bool compress = true);

    bool inProps(const std::string &key, set<string> &props);
    bool serializeFeature(OSMNode *object, std::string &serialized, std::string &indexStmt);
    bool serializeFeature(OSMWay *object, std::string &serialized, std::string &indexStmt);
    bool processRelation(OSMRelation *rel, const WayNodesMap &waysToNodesMap);

    std::string getName() const;
    GeometryType getType() const;
    std::string getTableStmt() const;
    std::string getIndexStmt() const;
    int getSkip(const OSMWay* way) const;

    /**
     * @brief isInterested
     * @param object
     * @return
     */
    template <typename T>
    bool isInterested(T* object) {
        if(kv_.size() == 0)
            return false;
        std::unordered_map<std::string, std::string> objTags;
        for(int i=0; i < (int)object->nTags; ++i) {
            std::string key = object->tags[i].key;
            std::string value = object->tags[i].value;
            objTags.insert({key, value});
        }
        for(const std::pair<std::string,std::set<std::string> > &entry : kv_) {
            if(objTags.count(entry.first) == 0)
                return false;
            if(!inProps(objTags[entry.first], kv_[entry.first]))
                return false;
        }
        return true;
    }
    /**
     * @brief storeFeature
     * @param object
     * @return
     */
    template<typename T>
    bool storeFeature(const T &object) {
        std::string geomCreateSql, indexSql;
        if(!this->serializeFeature(object, geomCreateSql, indexSql)) {
            return false;
        }
        writeGeomData(geomCreateSql, indexSql);
        return true;
    }

    void writeGeomData(const std::string &geomCreateSql, const std::string &indexSql) const;
    /**
     * @brief close
     */
    void close() {
        fclose(layerDesc_);
    }
    /**
     *
     */
    template <typename Obj>
    std::string applyCompression(Obj &obj, StoredType type) {
        if(compression_) {
            std::string compressed = obj.encode();
            compressed.insert(compressed.begin(), (char)(StoredType::COMPRESSED | type));
            return StringUtils::stringToHex(compressed.c_str(), compressed.size());
        } else {
            size_t len = wkbSize(obj);
            unsigned char buf[len+1];buf[0] = 0;
            exportToWkb<Obj>(obj, wkbNDR, buf+1);
            return StringUtils::stringToHex(reinterpret_cast<const char*>(buf), len+1);
        }
    }
     /**
     *
     */
    template <typename T>
    void getBboxStmt(Vertex::VertexId id, const T &geometry, std::string &indexStmt);
    /**
     * @brief makeSqlInsert
     * @param id
     * @param object
     * @param buf
     * @param len
     * @param stmt
     * @param area
     */
    template <typename T, typename G>
    void makeSqlInsert(Vertex::VertexId id, const T* object, const G &geometry, const std::string &data, std::string &stmt);
private:
    std::string getFuncName(const ComputedField &data) const;
    std::string getObjTag(const ComputedField &data) const;
    std::string getFieldType(const string &fieldFunction) const;
    std::string matchStyleGroup(const std::string &tag) const;
    /**
     * @brief serializePoint
     * @param object
     * @param serialized
     * @param indexStmt
     * @return
     */
    bool serializePoint(const OSMNode *object, std::string &serialized, std::string &indexStmt);
    /**
     * @brief serializeLine
     * @param way
     * @param serialized
     * @return
     */
    bool serializeLine(const OSMWay* way, std::string &serialized, std::string &indexStmt);
    /**
     * @brief serializePolygon
     * @param way
     * @param serialized
     * @return
     */
    bool serializePolygon(const OSMWay* way, std::string &serialized, std::string &indexStmt);
    /**
     * Check if we cannot split the object (because it will damage drawing properties)
     * for waterways for example
     */
    template<typename T>
    bool isNonSplittable(const T* object) {
        for(int i=0; i < (int)object->nTags; ++i) {
            std::string key = object->tags[i].key;
            if(nonSplittable_.count(key))
                return true;
        }
        return false;
    }
    /**
     * @brief extractTagValue
     * @param key
     * @param object
     * @return
     */
    template <typename T>
    std::string extractTagValue(const std::string &key, const T* object) {
        for(unsigned int i=0; i < object->nTags; ++i) {
            if (object->tags[i].key == key) {
                std::string value = object->tags[i].value;
                if (value.find(StringConsts::SEPARATOR) == std::string::npos) {
                    return StringUtils::escape(value);
                }
            }
        }
        return "";
    }
    /**
     * @brief computeFunctionVal
     * @param geometry
     * @param funcName
     * @param argument
     * @return
     */
    template <typename G>
    std::string computeFunctionVal(const G &geometry, const std::string &funcName, const std::string &argument) {
        if(funcName == "areaFunction")
            return lexical_cast(geometry.getArea());
        if(funcName == "styleGroupFunction")
            return matchStyleGroup(argument);
        assert(false);
        return "";
    }
};

/**
 * @brief The TilePlugin class
 */
class TilePlugin : public Plugin {
private:
    typedef Edge::EdgeId EdgeId;
    typedef Vertex::VertexId VertexId;
    typedef google::sparse_hash_map<VertexId, std::vector<VertexId>, std::hash<VertexId> > WayNodesMap;
    typedef google::sparse_hash_set<VertexId, std::hash<Vertex> > VertexSet;
    typedef std::map<int, int> NodesInWayMap;
private:
    int currPassId_;
    VertexSet seenWays_;
    std::string outputFileName_;
    std::vector<TileLayer*> layers_;
    std::vector<TileLayer*> relLayers_;
    VertexSet waysFromMultipolygons_;
    WayNodesMap waysToNodesMap_;
    NodesInWayMap nodesInWay_;
    VertexToPointIndexSql vpIndex_;
    // path to world_water.sqlite
    std::string worldWaterPath_;
private:
    void readLayers(const std::string &propName, XmlConfig &xConf, std::vector<TileLayer*> &layerList);
    void readComputedFields(const string &lName, XmlConfig &conf, TileLayer::ComputedFieldMap &map) const;
    void storeWorldWater();
public:
    TilePlugin(const std::string& outputFile, const std::string &configFile, bool compress = true);
public:
    virtual void init();
    virtual int getPassNumber();
    virtual void notifyPassNumber(const int currPassId);
    virtual void notifyNode(OSMNode* node);
    virtual void notifyWay(OSMWay* w);
    virtual void notifyRelation(OSMRelation* rel);
    virtual void notifyEndParsing();
    virtual void finalize();
    virtual void cleanUp();
    void initTileLayerFiles();
    virtual std::vector<std::string> getFileNamesToRead() const;
    bool isValidWay(Edge::EdgeId wayId);
    void setWorldWaterPath(const std::string& path);
};
