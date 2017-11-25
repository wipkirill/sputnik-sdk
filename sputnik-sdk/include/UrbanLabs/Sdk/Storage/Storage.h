#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/Utils/Timer.h>
#include <UrbanLabs/Sdk/Utils/Types.h>
#include <UrbanLabs/Sdk/Utils/URL.h>
#include <UrbanLabs/Sdk/Utils/Properties.h>
#include <UrbanLabs/Sdk/GraphCore/Vertices.h>
#include <UrbanLabs/Sdk/GraphCore/Edges.h>
#include <UrbanLabs/Sdk/SqlModels/Tag.h>
#include <UrbanLabs/Sdk/Storage/SqlQuery.h>
#include <UrbanLabs/Sdk/Storage/QueryConditions.h>
#include <UrbanLabs/Sdk/Storage/DatabaseConnection.h>

class VertexToPointIndexSql;
/**
 * @brief The VertexToPointIndexSqlite class
 */
class VertexToPointIndexSql {
public:
    class Initializer {
    private:
        URL url_;
        Properties props_;
    public:
        Initializer(const URL &url, const Properties &props);
        bool init(VertexToPointIndexSql &storage) const;
    };
    class Destructor {
    public:
        static bool release(VertexToPointIndexSql &storage);
    };
private:
    std::string idCol_;
    std::string table_;
    std::unique_ptr<DbConn> conn_;
    std::unique_ptr<PrepStmt> selectStmt_;
    std::unique_ptr<PrepStmt> insertStmt_;
    std::unique_ptr<VertexToPointIndexSql> auxIndex_;
    bool hasAuxIndex_;
public:
    bool open(const URL &url, const Properties &props);
    bool close();
    bool buildIndex();
    bool getPoint(const Vertex &v, Point& foundPoint);
    bool getPoints(const std::vector<Vertex::VertexId> &v,
                   std::vector<Point> &foundPoint, std::vector<int8_t> &found);
    bool insertPoint(Vertex::VertexId id, Point::CoordType lat, Point::CoordType lon);
    bool getBoundingBox(std::vector<Point>& bbox) const;
private:
    bool getPointsInternal(const std::vector<Vertex::VertexId> &vs,
                           std::vector<Point> &pt, std::vector<int8_t> &found);
};
/**
 * @brief The TagStorage class
 */
class TagStorage {
public:
    class Initializer {
    private:
        URL url_;
        Properties props_;
    public:
        Initializer(const URL &url, const Properties &props);
        bool init(TagStorage &storage) const;
    };
    class Destructor {
    public:
        static bool release(TagStorage &storage);
    };
protected:
    std::unique_ptr<DbConn> conn_;
public:
    bool open(const URL &url, const Properties &props);

    bool close();

    bool getObjectsInGroups(const std::string &table,
                            const std::vector<Vertex::VertexId> &groupIds,
                            std::vector<std::vector<Vertex::VertexId> > &ids);

    bool getGroupsForObjects(const std::string &table,
                             const std::vector<Vertex::VertexId> &ids,
                             std::vector<std::vector<Vertex::VertexId> > &groupIds);

    bool simpleSearch(const std::string &table, ConditionContainer &conds,
                      int offset, int limit, std::vector<TagList> &tags, bool joinTags = true);

    bool fulltextSearch(const std::string &table, const ConditionContainerFullText &conds,
                        int offset, int limit, vector<TagList> &tags);

    bool write(const std::string &table, const TagList &list);
    bool getFixedTags(const std::string &table, const std::vector<Vertex::VertexId> &ids,
                      std::vector<std::vector<KeyValuePair> > &tags);
protected:
    bool compress(const std::string &table, const std::vector<std::string> &tokens,
                  std::vector<std::string> &hashes);

    bool buildAndBindQuery(const std::string &table,
                           const vector<unique_ptr<Condition> > &conds,
                           int offset, int limit, std::unique_ptr<PrepStmt> &stmt);

    bool buildAndBindFulltextQuery(const std::string &table,
                                   const ConditionContainerFullText &conds,
                                   int offset, int limit, int iter, std::unique_ptr<PrepStmt> &stmt);

    bool buildAndBindFixedQuery(const string &table, const vector<Vertex::VertexId> &ids, unique_ptr<PrepStmt> &stmt);
};
/**
 * @brief The GeometryIndexSqlite class
 */
class GeometryIndexSql {
public:
    typedef Vertex::VertexId VertexId;
    typedef Point::CoordType CoordType;
private:
    std::unique_ptr<DbConn> conn_;
    std::unique_ptr<PrepStmt> selectStmt_;
    std::unique_ptr<PrepStmt> insertStmt_;
    bool compressGoogle_;
private:
    std::unique_ptr<uint8_t[]> serialize(const std::vector<Point> &points, size_t &len) const;
    std::string serializeGoogle(const std::vector<Point> &points) const;
    std::vector<Point> deserializeGoogle(const char * compressed, size_t len) const;
    std::vector<Point> deserialize(const std::string &compressed) const;
public:
    GeometryIndexSql();
    ~GeometryIndexSql();
    bool open(const URL &url, const Properties &props);
    bool close();
    virtual bool insertGeometry(const VertexId id1, const VertexId id2,
                                const bool fixed, std::vector<Point> &points);
    virtual bool findGeometry(const VertexId id1, const VertexId id2, const Point &target,
                              const bool fixed, std::vector<Point> &geometry);

    void setCompression(bool compress);
};
/**
 ** Use this index to read data from spatial databases
*/
class AuxiliarySpatialStorage {
public:
    typedef Vertex::VertexId VertexId;
    typedef Point::CoordType CoordType;
private:
    std::unique_ptr<DbConn> conn_;
    std::unique_ptr<PrepStmt> selectStmt_;
public:
    AuxiliarySpatialStorage();
    ~AuxiliarySpatialStorage();
    bool close();
    bool open(const URL &url, const Properties &props);
    bool findGeometry(const Point &hiLeft, const Point &lowRight, std::vector<string> &geometries);
};
/**
 * @brief The SqliteStream class
 */
class SqlStream {
private:
    // state of the stream
    int currColId_;
    std::string table_;
    // database connection
    std::unique_ptr<DbConn> conn_;
    std::unique_ptr<PrepStmt> stmt_;
public:
    SqlStream();
public:
    ~SqlStream();
    bool open(const URL &url, const Properties &props);
    bool close();
    int64_t getNumRows(const std::string &table) const;
    int64_t getNumRows() const;
    SqlStream &operator >> (int8_t &var);
    SqlStream &operator >> (uint8_t &var);
    SqlStream &operator >> (int64_t &var);
    SqlStream &operator >> (uint64_t &var);
    SqlStream &operator >> (int32_t &var);
    SqlStream &operator >> (uint32_t &var);
    SqlStream &operator >> (double &var);
    SqlStream &operator >> (float &var);
    SqlStream &operator >> (std::string &var);
    bool getNext();
    bool eof();
};

class TileStorage {
private:
    // database connection
    std::unique_ptr<DbConn> conn_;
    std::unique_ptr<PrepStmt> stmt_;
public:
    bool open(const URL &url, const Properties &props);
    bool close();
    std::string getTile(const std::string& zoom, const std::string& column, const std::string& row) const;
};
