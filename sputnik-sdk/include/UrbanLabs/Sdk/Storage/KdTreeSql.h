#pragma once

#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/Utils/ThreadSafe.h>
#include <UrbanLabs/Sdk/Utils/Timer.h>
#include <UrbanLabs/Sdk/Utils/Logger.h>
#include <UrbanLabs/Sdk/Storage/Storage.h>
#include <UrbanLabs/Sdk/GraphCore/BoundingBox.h>
#include <UrbanLabs/Sdk/GraphCore/NearestNeighbor.h>

class KdTreeSql {
public:
    class Initializer {
    private:
        URL url_;
        Properties props_;
        bool fetchData_;
    public:
        Initializer(const URL &url, const Properties &props, bool fetchData_);
        bool init(KdTreeSql &storage) const;
    };
    class Destructor {
    public:
        static bool release(KdTreeSql &storage);
    };
public:
    const static int PRECISION;
    typedef Point::CoordType CoordType;
    typedef OsmGraphCore::NearestPointResult NearestPointResult;
protected:
    std::string indexFileName_;
    std::string dataTablePostfix_;
    std::string dataFileName_;
    std::ofstream bulkStream_;
    std::ofstream bulkDataStream_;
    std::string dbFile_;
    std::string treeTableName_;
    bool createDataTable_;
    // database connection
    std::string idCol_;
    std::string table_;
    bool fetchData_;
    std::unique_ptr<DbConn> conn_;
    std::unique_ptr<PrepStmt> stmt_;
protected:
    KdTreeSql &operator = (const KdTreeSql &kd);
    KdTreeSql(const KdTreeSql &kd);
private:
    bool queryRtree(const Point &hiLeft, const Point &lowRight,
                    std::vector<VertexPoint> &results, std::vector<string> &data);
public:
    KdTreeSql();
    virtual ~KdTreeSql();
    bool open(const URL &url, const Properties &props, bool fetchData = true);
    bool openForBulkLoad(const std::string& dbFile, const std::string& treeTableName, bool createDataTable = true);
    std::vector<std::string> getTableNamesToImport() const;

    bool insertBulk(const VertexPoint &vertexPoint,const std::string &separator,const std::string &data = "");
    bool insertBulkSql(const VertexPoint &vertexPoint,const std::string &separator,
                       const std::string &data = "");
    bool findNearestVertex(const Point &pt, NearestPointResult &result, std::string &data);
    bool findAllInBoundingBox(const Point &hiLeft, const Point &lowRight,
                              std::vector<VertexPoint> &results, std::vector<string> &data);
    bool getPoints(const std::vector<Vertex::VertexId> &vs,std::vector<Point> &pt, std::vector<int8_t> &found);
    void closeBulk();
    void cleanUp();
    void close();
};
