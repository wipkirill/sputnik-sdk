// KdTreeSpatial.cpp
//
#include <UrbanLabs/Sdk/GraphCore/Vertices.h>
#include <UrbanLabs/Sdk/Utils/StringUtils.h>
#include <UrbanLabs/Sdk/Storage/KdTreeSql.h>
#include <UrbanLabs/Sdk/Storage/ConnectionsManager.h>

using namespace std;

const int KdTreeSql::PRECISION = 7;

//------------------------------------------------------------------------------
// KdTreeSqlite
//------------------------------------------------------------------------------
KdTreeSql::Initializer::Initializer(const URL &url, const Properties &props, bool fetchData)
    : url_(url), props_(props), fetchData_(fetchData) {
    ;
}
/**
 * @brief KdTreeSql::Initializer::init
 * @param storage
 */
bool KdTreeSql::Initializer::init(KdTreeSql &storage) const {
    return storage.open(url_, props_, fetchData_);
}
/**
 * @brief KdTreeSql::Destructor::release
 * @param storage
 */
bool KdTreeSql::Destructor::release(KdTreeSql &storage) {
    storage.close();
    return true;
}
/**
 * @brief KdTreeSql::KdTreeSql
 */
KdTreeSql::KdTreeSql(){
    dataTablePostfix_ = "_data";
}
/**
 * @brief KdTreeSpatial::~KdTreeSpatial
 */
KdTreeSql::~ KdTreeSql() {
    close();
}
/**
 * @brief KdTreeSqlite::open
 * @param url
 * @param props
 * @return
 */
bool KdTreeSql::open(const URL &url, const Properties &props, bool fetchData) {
    fetchData_ = fetchData;
    conn_ = ConnectionsManager::getConnection(url, props);
    if(!conn_ || !conn_->open(url, props))
        return false;
    table_ = props.get("table");
    if (table_ == "") {
        LOGG(Logger::ERROR) << "[KDTREE SQLITE] No table name provided" << Logger::FLUSH;
        return false;
    }
    if(!conn_ || !conn_->primaryKey(table_, idCol_)) {
        LOGG(Logger::ERROR) << "[KDTREE SQLITE] No id column provided" << Logger::FLUSH;
        return false;
    }
    SqlQuery nearestPt = SqlQuery::q().select({table_+"."+idCol_,"minlat","minlon"})
                .from(table_).where("minlat>=? AND maxlat<=? AND minlon>=? AND maxlon<=?");
    if (fetchData) {
        string dataField = props.get("dataField");
        if (dataField == "")
            dataField = "data";

        SqlQuery inner = SqlQuery::q().select({table_+"."+idCol_})
                .from(table_).where("minlat>=? AND maxlat<=? AND minlon>=? AND maxlon<=?");

        vector<string> tables = {table_, table_+dataTablePostfix_};
        SqlQuery base = SqlQuery::q().select({table_+"."+idCol_,"minlat","minlon", dataField})
                .from(tables).in(table_+"."+idCol_, inner)
                .where(table_+"."+idCol_ +"="+table_+dataTablePostfix_+"."+idCol_);
        nearestPt = base;
    }
    if(!conn_->prepare(nearestPt.toString(), stmt_)) {
        LOGG(Logger::ERROR) << "[KDTREE SQLITE] Failed to prepare select stmt" << Logger::FLUSH;
        return false;
    }
    LOGG(Logger::INFO) << "[KDTREE SQLITE] :" << nearestPt.toString() << Logger::FLUSH;
    return true;
}
/**
 * @brief KdTreeSqlite::openForBulkLoad
 * @param treeTableName
 * @param createDataTable
 * @return
 */
bool KdTreeSql::openForBulkLoad(const string &dbFile, const string& treeTableName,
                                bool createDataTable) {
    dbFile_ = dbFile;
    treeTableName_ = treeTableName;
    createDataTable_ = createDataTable;

    indexFileName_ = dbFile_ + StringConsts::PT + treeTableName_;
    ifstream file(indexFileName_.c_str());
    if (file) {
        LOGG(Logger::ERROR) << "[KDTREE SQLITE] Bulk file aready exists" << Logger::FLUSH;
        return false;
    }
    bulkStream_.open(indexFileName_);
    bulkStream_.precision(9);
    if (!bulkStream_.is_open()) {
        LOGG(Logger::ERROR) << "[KDTREE SQLITE] Error opening bulk index file" << Logger::FLUSH;
        return false;
    }
    if (createDataTable_) {
        dataFileName_ = indexFileName_ + dataTablePostfix_;
        ifstream dataStream(dataFileName_.c_str());
        if (dataStream) {
            LOGG(Logger::ERROR) << "[KDTREE SQLITE] Bulk data file aready exists" << Logger::FLUSH;
            return false;
        }
        bulkDataStream_.open(dataFileName_);
        bulkDataStream_.precision(9);
        if (!bulkDataStream_.is_open()) {
            LOGG(Logger::ERROR) << "[KDTREE SQLITE] Error opening bulk data file" << Logger::FLUSH;
            return false;
        }
    }
    return true;
}
/**
 * @brief KdTreeSqlite::insertBulk
 * @param vertexPoint
 * @param separator
 * @param data
 * @return
 */
bool KdTreeSql::insertBulk(const VertexPoint &vertexPoint,const std::string &separator,
                           const std::string &data) {
    if(!bulkStream_.good()) {
        LOGG(Logger::ERROR) << "[KDTREE SQLITE] Error writing to bulk index" << Logger::FLUSH;
        return false;
    }

    Vertex::VertexId id = vertexPoint.getId();
    if(PRECISION == -1) {
        CoordType coords[2] = {vertexPoint.getPoint().lat(), vertexPoint.getPoint().lon()};
        // store id and coords
        bulkStream_ << id << separator << coords[0] << separator << coords[0] <<
                    separator << coords[1] << separator << coords[1]<< endl;
    } else {
        double fact = pow(10, PRECISION);
        int64_t coords[2] = {(int64_t)(vertexPoint.getPoint().lat()*fact), 
                            (int64_t)(vertexPoint.getPoint().lon()*fact)};
        // store id and coords
        bulkStream_ << id << separator << coords[0] << separator << coords[0] <<
                    separator << coords[1] << separator << coords[1] << endl;
    }
    if(createDataTable_) {
        if(!bulkDataStream_.good()) {
            LOGG(Logger::ERROR) << "[KDTREE SQLITE] Error writing to bulk data" << Logger::FLUSH;
            return false;
        }
        bulkDataStream_ << id << separator << data << endl;
    }
    return true;
}
/**
 * @brief KdTreeSqlite::findNearestVertex
 * @param vertexPoint
 * @param result
 * @return
 */
bool KdTreeSql::findNearestVertex(const Point &pt, NearestPointResult &result, string& data) {
    // initial bbox
    int attempts = 100;
    double bboxFactor = 0.0001;
    double half = bboxFactor / 2.0;

    // results
    vector<string> pointData;
    vector<VertexPoint> nearVertPoints;
    while(nearVertPoints.size() == 0 && attempts > 0) {
        Point hiLeft(pt.lat()+half, pt.lon()-half);
        Point lowRight(pt.lat()-half, pt.lon()+half);
        findAllInBoundingBox(hiLeft, lowRight, nearVertPoints, pointData);
        half *= 2.0;
        attempts--;
    }
    if(nearVertPoints.size() == 0) {
        LOGG(Logger::ERROR) << "[KDTREE SQLITE] Failed to find nearest neighbour" << Logger::FLUSH;
        return false;
    }
    vector<Point> nearestPts(nearVertPoints.size());
    for(int i = 0; i < (int)nearestPts.size(); ++i)
        nearestPts[i] = nearVertPoints[i].getPoint();
    size_t cId = closestPoint(pt, nearestPts);
    result.setTarget(VertexPoint(nearVertPoints[cId].getId(),nearVertPoints[cId].getPoint()));
    if (fetchData_) {
        data = pointData[cId];
    }
    return true;
}
/**
 * @brief findAllInBoundingBox
 * @param hiLeft
 * @param lowRight
 * @param results
 * @return
 */
bool KdTreeSql::findAllInBoundingBox(const Point &hiLeft, const Point &lowRight,
                                     std::vector<VertexPoint> &results, std::vector<string> &data) {
    // usual case when doesn't cross 0th meridian
    if(hiLeft.lon() <= lowRight.lon()) {
        if(!queryRtree(hiLeft, lowRight, results, data))
            return false;
    } else if(hiLeft.lon() > lowRight.lon()) {
        // crossing 0th meridian, make 2 requests
        LOGG(Logger::WARNING) << "crossing 0th meridian boundary, check bounding box!" << Logger::FLUSH;
        // from large latitude to 180
        Point pt1(max(hiLeft.lat(), lowRight.lat()), max(hiLeft.lon(), lowRight.lon()));
        Point pt2(min(hiLeft.lat(), lowRight.lat()), -180.0);

        if(!queryRtree(pt1, pt2, results, data))
            return false;
        // from 0 to small latutude
        Point pt3(max(hiLeft.lat(), lowRight.lat()), -180.0);
        Point pt4(min(hiLeft.lat(), lowRight.lat()), min(hiLeft.lon(), lowRight.lon()));

        vector<VertexPoint> addRes;
        vector<string> addData;
        if(!queryRtree(pt3, pt4, addRes, addData))
            return false;

        results.insert(results.end(), addRes.begin(), addRes.end());
        data.insert(data.end(), addData.begin(), addData.end());
    }
    return true;
}
/**
 * @brief KdTreeSqlite::queryRtree
 * @param hiLeft
 * @param lowRight
 * @param results
 * @param data
 * @return
 */
bool KdTreeSql::queryRtree(const Point &hiLeft, const Point &lowRight,
                           vector<VertexPoint> &results, vector<string> &data) {
    if(stmt_) {
        stmt_->reset();
        // bind min and max bbox coords
        double fact = pow(10, PRECISION);
        if(PRECISION == -1) {
            if(!stmt_->bind(lowRight.lat()) || !stmt_->bind(hiLeft.lat()))
                return false;
            if(!stmt_->bind(hiLeft.lon()) || !stmt_->bind(lowRight.lon()))
                return false;
        } else {
            if(!stmt_->bind((int64_t)(lowRight.lat()*fact)) || !stmt_->bind((int64_t)(hiLeft.lat()*fact)))
                return false;
            if(!stmt_->bind((int64_t)(hiLeft.lon()*fact)) || !stmt_->bind((int64_t)(lowRight.lon()*fact)))
                return false;
        }

        while(stmt_->step()) {
            // extract id
            Point::CoordType lat, lon;
            Vertex::VertexId id = stmt_->column_int64(0);
            if(PRECISION == -1) {
                // minlat field(see sql in init())
                lat = stmt_->column_double(1);
                // minlon field
                lon = stmt_->column_double(2);
            } else {
                // minlat field(see sql in init())
                lat = stmt_->column_int64(1)/fact;
                // minlon field
                lon = stmt_->column_int64(2)/fact;
            }
            results.push_back(VertexPoint(id, lat, lon));
            // if data field was requested, extract 5th column
            if(fetchData_) {
                string pData = stmt_->column_text(3);
                data.push_back(pData);
            }
        }
        return true;
    }
    return false;
}
/**
 * @brief KdTreeSqlite::getPoints
 * @param vs
 * @param pt
 * @param found
 * @return
 */
bool KdTreeSql::getPoints(const vector<Vertex::VertexId> &vs, vector<Point> &pt, vector<int8_t> &found) {
    found = vector<int8_t>(vs.size(), 0);

    std::unique_ptr<PrepStmt> stmt;
    SqlQuery query = SqlQuery::q().select({idCol_, "minlat", "minlon"}).
            from(table_).in(idCol_, vs.size());

    if(!query.isValid() || !conn_ || !conn_->prepare(query.toString(), stmt))
        return false;
    if(!stmt->bind(vs))
        return false;

    // record the all positions of a VertexId in the array
    vector<vector<int> > posArr;
    map<Vertex::VertexId, int> posMap;
    for(int i = 0, curr = -1; i < vs.size(); i++) {
        if(posMap.find(vs[i]) == posMap.end()) {
            posArr.push_back(vector<int>());
            posMap[vs[i]] = ++curr;
            posArr[curr].push_back(i);
        } else {
            posArr[posMap[vs[i]]].push_back(i);
        }
    }

    int totalRows = 0;
    double fact = pow(10, PRECISION);
    pt = vector<Point>(vs.size());
    while(stmt->step()) {
        Point::CoordType lat, lon;
        Vertex::VertexId id = stmt->column_int64(0);
        if(PRECISION == -1) {
            lat = stmt->column_double(1);
            lon = stmt->column_double(2);
        } else {
            lat = stmt->column_int64(1)/fact;
            lon = stmt->column_int64(2)/fact;
        }
        int pos = posMap[id];
        for(int i = 0; i < posArr[pos].size(); i++) {
            pt[posArr[pos][i]].setLatLon(lat, lon);
            found[posArr[pos][i]] = 1;
            totalRows++;
        }
    }
    if(totalRows < vs.size())
        return false;
    return true;
}

/**
 * @brief KdTreeSqlite::getTableNamesToImport
 * @return
 */
vector<string> KdTreeSql::getTableNamesToImport() const {
    vector<string> tables = {treeTableName_};
    if (createDataTable_)
        tables.push_back(treeTableName_+ dataTablePostfix_);
    return tables;
}
/**
 * @brief KdTreeSqlite::closeBulk
 */
void KdTreeSql::closeBulk() {
    bulkStream_.close();
    if (createDataTable_)
        bulkDataStream_.close();
}
/**
 * @brief KdTreeSqlite::cleanUp
 */
void KdTreeSql::cleanUp() {
    std::remove(indexFileName_.c_str());
    if (createDataTable_)
        std::remove(dataFileName_.c_str());
}
/**
 * @brief KdTreeSqlite::unload
 */
void KdTreeSql::close() {
    stmt_.reset(0);
    conn_.reset(0);
}
