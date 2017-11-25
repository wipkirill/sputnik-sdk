// Storage.cpp
//
#include <cstring>
#include <UrbanLabs/Sdk/Storage/Storage.h>
#include <UrbanLabs/Sdk/Storage/ConnectionsManager.h>
#include <UrbanLabs/Sdk/Utils/PlatformUtils.h>
#include <UrbanLabs/Sdk/Utils/GPolyEncode.h>
#include <UrbanLabs/Sdk/Storage/SqlConsts.h>
#include <UrbanLabs/Sdk/Storage/QueryConditions.h>

using namespace std;

//--------------------------------------------------------------------------------------------------
// VertexToPointIndexSql
//--------------------------------------------------------------------------------------------------
/**
 * @brief VertexToPointIndexSqlite::init
 * @param indexFile
 * @param createIfMissing
 */
bool VertexToPointIndexSql::open(const URL &url, const Properties &props) {
    if(!close()) {
        return false;
    } else {
        conn_ = ConnectionsManager::getConnection(url, props);
        if(!conn_ || !conn_->open(url, props))
            return false;

        // create the table if it doesn't exist
        string query = SqlConsts::CREATE_VP_TABLE;
        bool create = lexical_cast<int32_t>(props.get("create"));
        if(create) {
            if(!conn_->exec(query))
                return false;
        }
        if(!conn_->exec(SqlConsts::PRAGMAS))
            return false;
        // TODO: in case of nosql storage this needs to be redone
        if(!conn_->beginTransaction())
            return false;

        table_ = props.get("table");
        string insertQ = "INSERT INTO "+table_+" VALUES (?, ?, ?);";
        if(!conn_->prepare(insertQ, insertStmt_))
            return false;

        if(!conn_ || !conn_->primaryKey(table_, idCol_))
            return false;

        string selectQ = SqlQuery::q().select({"lat", "lon"}).from(table_).where(idCol_+"=?").toString();
        if(!conn_->prepare(selectQ, selectStmt_))
            return false;

        return true;
    }
    return true;
}
/**
 * @brief VertexToPointIndexSql::close
 * @return
 */
bool VertexToPointIndexSql::close() {
    table_ = "";
    idCol_ = "";
    insertStmt_.reset(0);
    selectStmt_.reset(0);
    if(conn_ && !conn_->close())
        return false;
    conn_.reset(0);
    return true;
}
/**
 * @brief VertexToPointIndexSqlite::setUp
 */
bool VertexToPointIndexSql::buildIndex() {
    LOGG(Logger::INFO) << "[VERTEX TO POINT] Creating index..." << Logger::FLUSH;
    string query = SqlQuery::ci(SqlConsts::VP_TABLE,{"vertexid"});
    if(!conn_ || !conn_->exec(query)) {
        return false;
    }
    return true;
}
/**
 * @brief VertexToPointIndexSqlite::getPoint
 * @param v
 * @return
 */
bool VertexToPointIndexSql::getPoint(const Vertex &v, Point &foundPoint) {
    if(!selectStmt_ || !selectStmt_->reset())
        return false;

    if(!selectStmt_->bind(v.getId()))
        return false;

    if(!selectStmt_->step()) {
        LOGG(Logger::ERROR) << "Vertex point " << v.getId() << " not found" << Logger::FLUSH;
        return false;
    }

    Point::CoordType lat = selectStmt_->column_double(0);
    Point::CoordType lon = selectStmt_->column_double(1);
    foundPoint.setLatLon(lat, lon);
    return true;
}
/**
 * Returns false if couldn't find all the points
 *
 * @brief VertexToPointIndexSqlite::getPoints
 * @param vs
 * @param pt
 * @return
 */
bool VertexToPointIndexSql::getPoints(const vector<Vertex::VertexId> &vs,
                                      vector<Point> &pt, vector<int8_t> &found) {
    int rows = 999, part = (vs.size()+rows-1)/rows;

    bool ret = true;
    for(int i = 0; i < part; i++) {
        vector<Point> tmpPt;
        vector<int8_t> tmpFound;
        int from = i*rows, to = min((i+1)*rows, (int)vs.size());
        vector<Vertex::VertexId> tmpV(vs.begin()+from,vs.begin()+to);

        if(!getPointsInternal(tmpV,tmpPt,tmpFound)) {
            ret = false;
        }

        pt.insert(pt.end(),tmpPt.begin(),tmpPt.end());
        found.insert(found.end(),tmpFound.begin(),tmpFound.end());
    }

    return ret;
}
/**
 * @brief VertexToPointIndexSql::getPointsInternal
 * @param vs
 * @param pt
 * @param found
 * @return
 */
bool VertexToPointIndexSql::getPointsInternal(const vector<Vertex::VertexId> &vs,
                                              vector<Point> &pt, vector<int8_t> &found)
{
    found = vector<int8_t>(vs.size(), 0);

    unique_ptr<PrepStmt> stmt;
    SqlQuery query = SqlQuery::q().select({idCol_, "lat", "lon"}).
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
    pt = vector<Point>(vs.size());
    while(stmt->step()) {
        Vertex::VertexId id = stmt->column_int64(0);
        Point::CoordType lat = stmt->column_double(1);
        Point::CoordType lon = stmt->column_double(2);
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
 * @brief VertexToPointIndexSqlite::getBoundingBox
 * @param bbox
 * @return
 */
bool VertexToPointIndexSql::getBoundingBox(vector<Point> &bbox) const {
    string q = SqlQuery::q().select({"max(lat)", "min(lon)", "min(lat)","max(lon)"}).from(table_).toString();
    unique_ptr<PrepStmt> stmt;
    if(!conn_ || !conn_->prepare(q, stmt))
        return false;
    if(!stmt->step()) {
        LOGG(Logger::INFO) << "Bounding box not found" << Logger::FLUSH;
        return false;
    }

    Point leftTop, rightBottom;
    leftTop.setLatLon(stmt->column_double(0), stmt->column_double(1));
    rightBottom.setLatLon(stmt->column_double(2), stmt->column_double(3));
    bbox.push_back(leftTop);
    bbox.push_back(rightBottom);
    return true;
}
/**
 * @brief VertexToPointIndexSqlite::insertPoint
 * @param id
 * @param lat
 * @param lon
 */
bool VertexToPointIndexSql::insertPoint(Vertex::VertexId id, Point::CoordType lat, Point::CoordType lon) {
    if(!insertStmt_ || !insertStmt_->reset())
        return false;
    if(!insertStmt_->bind(id) || !insertStmt_->bind(lat) || !insertStmt_->bind(lon))
        return false;
    if(!insertStmt_->exec())
        return false;
    return true;
}

VertexToPointIndexSql::Initializer::Initializer(const URL &url, const Properties &props)
    : url_(url), props_(props){;}

/**
 * @brief VertexToPointIndexSqlite::Initializer::init
 * @param storage
 */
bool VertexToPointIndexSql::Initializer::init(VertexToPointIndexSql &storage) const {
    if(!storage.open(url_, props_)) {
        LOGG(Logger::ERROR) << "couldn't init vp index" << Logger::FLUSH;
        return false;
    }
    return true;
}
//--------------------------------------------------------------------------------------------------
// GeometryIndexSql
//--------------------------------------------------------------------------------------------------
/**
 * @brief GeometryIndexSql::GeometryIndexSql
 */
GeometryIndexSql::GeometryIndexSql(): compressGoogle_(true) {
    ;
}
/**
 * @brief GeometryIndexSql::~GeometryIndexSql
 */
GeometryIndexSql::~GeometryIndexSql() {
    close();
}
/**
 * @brief GeometryIndexSql::setCompression
 * @param compress
 */
void GeometryIndexSql::setCompression(bool compress) {
    compressGoogle_ = compress;
}
/**
 * @brief GeometryIndexSqlite::init
 * @param indexFile
 * @param createIfMissing
 */
bool GeometryIndexSql::open(const URL &url, const Properties &props) {
    if(!close()) {
        return false;
    } else {
        conn_ = ConnectionsManager::getConnection(url, props);
        if(!conn_ || !conn_->open(url, props))
            return false;

        // create the table if it doesn't exist
        string table = props.get("table");
        if(table == "")
            table = SqlConsts::GEOMETRY_TABLE;

        string query = SqlConsts::CREATE_GEOMETRY;
        bool create = lexical_cast<int32_t>(props.get("create"));
        if(create) {
            if(!conn_->exec(query))
                return false;
        }
        if(!conn_->exec(SqlConsts::PRAGMAS))
            return false;
        // TODO: in case of nosql storage this needs to be redone
        if(!conn_->beginTransaction())
            return false;
        if(!conn_->prepare(SqlConsts::INSERT_GEOMETRY, insertStmt_))
            return false;

        SqlQuery select = SqlQuery::q().select({"geom"}).from(SqlConsts::GEOMETRY_TABLE).where("ver1=? AND ver2=?");
        if(!conn_->prepare(select.toString(), selectStmt_))
            return false;
        return true;
    }
}
/**
 * @brief GeometryIndexSqlite::tearDown
 */
bool GeometryIndexSql::close() {
    insertStmt_.reset(0);
    selectStmt_.reset(0);
    if(conn_ && !conn_->close())
        return false;
    conn_.reset(0);
    return true;
}
/**
 * @brief GeometryIndexSqlite::insertGeometry
 * @param id1
 * @param id2
 * @param fixed
 * @param points
 * @return
 */
bool GeometryIndexSql::insertGeometry(const VertexId id1, const VertexId id2,
                                      const bool fixed, vector<Point> &points) {
    Edge::EdgeKey id;
    if(fixed)
        id = edgeKeyFixed(Vertex(id1), Vertex(id2));
    else
        id = edgeKey(Vertex(id1), Vertex(id2));

    if(!fixed && id1 > id2) {
        reverse(points.begin(),points.end());
    }

    if(!insertStmt_ || !insertStmt_->reset())
        return false;
    if(!insertStmt_->bind(id.first) || !insertStmt_->bind(id.second))
        return false;

    if(compressGoogle_) {
        string serialized = serializeGoogle(points);
        if(!insertStmt_->bind(serialized))
            return false;
    } else {
        size_t len = 0;
        unique_ptr<uint8_t[]> serialized = serialize(points, len);
        if(!insertStmt_->bind(serialized.get(), len))
            return false;
    }
    if(!insertStmt_->exec())
        return false;
    return true;
}
/**
 * @brief GeometryIndexSqlite::serializePoints
 * @param points
 * @param len
 * @return
 */
unique_ptr<uint8_t[]> GeometryIndexSql::serialize(const vector<Point> &points, size_t &len) const {
    vector<Point::CoordType> coords;
    for (size_t i = 0; i < points.size();i++){
        coords.push_back(points[i].lat());
        coords.push_back(points[i].lon());
    }
    len = 2*sizeof(Point::CoordType)*points.size()+1;
    unique_ptr<uint8_t[]> out(new uint8_t[len]);
    // set no compression flag
    char comprFlag = 0;
    memcpy(out.get()+0,(uint8_t*)(&comprFlag), sizeof(char));
    for(size_t i = 0, off = 1; i < coords.size(); i++, off += sizeof(Point::CoordType)) {
        memcpy(out.get()+off, (uint8_t*)(&coords[i]), sizeof(Point::CoordType));
    }
    return out;
}
/**
 * @brief GeometryIndexSql::serializeGoogle
 * @param points
 * @param len
 * @return
 */
string GeometryIndexSql::serializeGoogle(const vector<Point> &points) const {
    GPolyEncoder encoder;
    string encoded = encoder.encodePoints(points);
    encoded.insert(encoded.begin(), StoredType::COMPRESSED);
    return encoded;
}
/**
 * @brief GeometryIndexSql::deserializeGoogle
 * @param compressed
 * @return
 */
vector<Point> GeometryIndexSql::deserializeGoogle(const char *compressed, size_t len) const{
    GPolyEncoder encoder;
    return encoder.decodePoints(compressed, len);
}
/**
 * @brief GeometryIndexSqlite::deserialize
 * @param compressed
 * @param len
 * @return
 */
vector<Point> GeometryIndexSql::deserialize(const string &compressed) const {
    vector<Point> points;
    size_t len = compressed.size();
    if(len > 1) {
        const char *out = compressed.c_str();
        if (out[0] & StoredType::COMPRESSED)
            return deserializeGoogle(out+1, len-1);

        vector<double> vec(len/(sizeof(Point::CoordType)), 0);
        for(size_t i = 0, off = 1; i < vec.size(); i++, off += sizeof(Point::CoordType)) {
            memcpy((uint8_t*)(&vec[i]), out+off, sizeof(Point::CoordType));
        }

        for (size_t i = 0; i+1 < vec.size(); i += 2){
            points.push_back(Point(vec[i],vec[i+1]));
        }
    } else
        LOGG(Logger::WARNING) << "[GEOMETRY INDEX] Nothing to desirialize" << Logger::FLUSH;
    return points;
}
/**
 * @brief GeometryIndexSqlite::findGeometry
 * @param id1
 * @param id2
 * @param target
 * @param geometry
 */
bool GeometryIndexSql::findGeometry(const VertexId id1, const VertexId id2,
                                    const Point &target, const bool fixed,
                                    vector<Point> &geometry) {
    Edge::EdgeKey key;
    if(fixed)
        key = edgeKeyFixed(Vertex(id1),Vertex(id2));
    else
        key = edgeKey(Vertex(id1), Vertex(id2));

    if(!selectStmt_ || !selectStmt_->reset())
        return false;

    if(!selectStmt_->bind(key.first) || !selectStmt_->bind(key.second))
        return false;

    vector<Point> closestGeom;
    Point::PointDistType currDistance = numeric_limits<Point::PointDistType>::max();
    while(selectStmt_->step()) {
        string compressed = selectStmt_->column_blob(0);
        if(compressed == "") {
            LOGG(Logger::WARNING) << "[GEOMETRY INDEX] empty geometry for " << id1 << "->" << id2 << Logger::FLUSH;
            return false;
        }

        // we are looking for the closest geometry to target point
        vector<Point> tmpGeom = deserialize(compressed);
        int nearestIndex = closestPoint(target, tmpGeom);
        Point::PointDistType tmpDistance = manhattanDistance(target, tmpGeom[nearestIndex]);
        if(currDistance-tmpDistance > 0) {
            currDistance = tmpDistance;
            closestGeom = tmpGeom;
        }
    }
    // check if the geometry was found
    if(closestGeom.size() == 0) {
        LOGG(Logger::ERROR) << "[GEOMETRY INDEX] Error geometry not found " << id1 << "->"<< id2 << Logger::FLUSH;
        return false;
    }
    // reverse inner points
    if(id1 > id2 && !fixed) {
        reverse(closestGeom.begin(), closestGeom.end());
    }
    // fill in result
    geometry.insert(geometry.end(), closestGeom.begin(), closestGeom.end());
    return true;
}
//--------------------------------------------------------------------------------------------------
// TagStorage
//--------------------------------------------------------------------------------------------------
/**
 * @brief TagStorage::init
 * @param indexFile
 * @param createIfMissing
 * @return
 */
bool TagStorage::open(const URL &url, const Properties &props) {
    // prepare database connection
    if(conn_ && !conn_->close())
        return false;
    conn_ = ConnectionsManager::getConnection(url, props);
    if(!conn_ || !conn_->open(url, props))
        return false;
    return true;
}
/**
 * @brief TagStorage::close
 */
bool TagStorage::close() {
    if(conn_ && !conn_->close())
        return false;
    conn_.reset(0);
    return true;
}
/**
 * @brief TagStorage::buildAndBindQuery
 * @param table
 * @param conds
 * @param stmt
 * @return
 */
bool TagStorage::buildAndBindQuery(const string &table, const vector<unique_ptr<Condition> > &conds,
                                   int offset, int limit, unique_ptr<PrepStmt> &stmt) {
    Timer timer;

    // build the query
    string idCol;
    if(!conn_ || !conn_->primaryKey(table, idCol)) {
        LOGG(Logger::ERROR) << "Table "+table+" doesn't have a primary key" << Logger::FLUSH;
        return false;
    }

    // base query
    SqlQuery all = SqlQuery::q().select({"DISTINCT "+idCol}).from(table), objects = all;
    for(int i = 0; i < conds.size(); i++) {
        conds[i]->prepare(conn_, table);
        if(!conds[i]->isIntersectable()) {
            SqlQuery tmp = conds[i]->apply(all);
            objects = objects.intersect(tmp);
        } else {
            objects = conds[i]->apply(objects);
        }
    }

    objects = objects.offset(offset).limit(limit);
    LOGG(Logger::INFO) << objects.toString() << Logger::FLUSH;

    // join with the spatial index to get the points
    SqlQuery result;

    // detect key compression
    string keyHashTable = table+"_tag_hash";
    bool hasKeyCompr = conn_->existsTable(keyHashTable);
    if(hasKeyCompr) {
        string keyCol = keyHashTable+".tag", hashCol = table+".tag", keyHashCol = keyHashTable+".hash";
        result = result.select({table+"."+idCol,keyCol,"value"}).from(table).from(keyHashTable).
                in(table+"."+idCol, objects).where(keyHashCol+"="+hashCol).orderAsc(table+"."+idCol);
    } else {
        result = result.select({table+"."+idCol,"tag","value"}).from(table).
                in(table+"."+idCol, objects).orderAsc(table+"."+idCol);
    }
    LOGG(Logger::DEBUG) << "[TAGSEARCH]" << result.toString() << Logger::FLUSH;

    // prepare statement
    if(!result.isValid() || limit == 0 || !conn_ || !conn_->prepare(result.toString(), stmt))
        return false;

    // bind statements
    for(int i = 0; i < conds.size(); i++) {
        if(!conds[i]->bind(stmt))
            return false;
    }

    timer.stop();
    LOGG(Logger::INFO) << "Building and binding query: " << timer.getElapsedTimeSec() << " sec." << Logger::FLUSH;
    return true;
}
/**
 * @brief TagStorage::buildAndBindQuery
 * @param table
 * @param conds
 * @param stmt
 * @return
 */
bool TagStorage::buildAndBindFulltextQuery(const string &table,
                                           const ConditionContainerFullText &cont,
                                           int offset, int limit, int iter, unique_ptr<PrepStmt> &stmt) {
    // build the query
    string idCol;
    if(!conn_ || !conn_->primaryKey(table+"_fulltext", idCol)) {
        LOGG(Logger::ERROR) << "Table "+table+" doesn't have a primary key" << Logger::FLUSH;
        return false;
    }

    // base query
    string fullText = table+"_fulltext";
    SqlQuery objects = SqlQuery::q().select({"DISTINCT "+idCol}).from(fullText);
    objects = cont.apply(objects);
    objects = objects.orderAsc(idCol).offset(offset).limit(limit);

    LOGG(Logger::INFO) << objects.toString() << Logger::FLUSH;

    if(!objects.isValid() || limit == 0 || !conn_ || !conn_->prepare(objects.toString(), stmt))
        return false;

    // decompress tokens
    vector<string> found;
    map<string, vector<string> > tokens = cont.getCompressedTokens();
    if(tokens.size() > 0) {
        vector<string> toFind;
        for(auto entry : tokens) {
            for(const string &token : entry.second) {
                toFind.emplace_back(token);
            }
        }
        if(!compress(table, toFind, found))
            return false;

        int i = 0;
        for(const auto entry : tokens) {
            for(int j = 0; j < entry.second.size(); j++, i++) {
                found[i] = entry.first+":"+found[i];
            }
        }
    }

    // combine tokens all together
    vector<string> approx = cont.getNonCompressedTokens();
    found.insert(found.end(), approx.begin(), approx.end());

    // add location if its required
    if(cont.hasLocation()) {
        string token = lexical_cast(cont.getLocationToken(iter));
        if(token.size() > 0)
            found.emplace_back("curve:"+token+"*");
    }

    string toMatch = "";
    for(int i = 0; i < found.size(); i++) {
        toMatch.append(found[i]);
        if(i+1 < found.size())
            toMatch.append(" ");
    }

    // bind all the tokens
    if(!stmt->bind(toMatch))
        return false;

    return true;
}
/**
 * @brief TagStorage::buildAndBindFixedQuery
 * @param table
 * @param conds
 * @param offset
 * @param limit
 * @param stmt
 * @return
 */
bool TagStorage::buildAndBindFixedQuery(const string &table,
                                        const vector<Vertex::VertexId> &ids,
                                        unique_ptr<PrepStmt> &stmt) {
    // build the query
    string idCol;
    if(!conn_ || !conn_->primaryKey(table+"_tag_fixed", idCol)) {
        LOGG(Logger::ERROR) << "Table "+table+"_tag_fixed doesn't have a primary key" << Logger::FLUSH;
        return false;
    }

    // base query
    string keyHashTable = table+"_tag_hash", fixedTable = table+"_tag_fixed";
    string keyCol = keyHashTable+".tag", hashCol = keyHashTable+".hash", keyHashCol = fixedTable+".tag";
    SqlQuery objects = SqlQuery::q().select({idCol, keyCol}).from(keyHashTable).from(fixedTable).
            where(hashCol+"="+keyHashCol).in(idCol, ids.size()).orderAsc(idCol);

    LOGG(Logger::INFO) << objects.toString() << Logger::FLUSH;

    if(!objects.isValid() || !conn_ || !conn_->prepare(objects.toString(), stmt))
        return false;

    // bind statements
    if(!stmt->bind(ids))
        return false;

    return true;
}
/**
 * @brief TagStorage::compress
 * @param table
 * @param tokens
 * @param hashes
 * @return
 */
bool TagStorage::compress(const string &table, const vector<string> &tokens, vector<string> &hashes) {
    unique_ptr<PrepStmt> stmt;
    SqlQuery query = SqlQuery::q().select({"tag","hash"}).from(table+"_tag_hash").in("tag", tokens.size());
    if(!conn_ || !conn_->prepare(query.toString(), stmt))
        return false;

    if(!stmt->bind(tokens))
        return false;

    map<string,string> found;
    while(stmt->step()) {
        string tag = stmt->column_text(0);
        string hash = stmt->column_text(1);
        found[tag] = hash;
    }

    hashes = vector<string>(tokens.size());
    for(int i = 0; i < tokens.size(); i++) {
        if(found.find(tokens[i]) == found.end())
            return false;
        hashes[i] = found[tokens[i]];
    }

    return true;
}

/**
 * @brief TagStorage::matchFullText
 * @param table
 * @param cont
 * @param offset
 * @param limit
 * @param ids
 * @return
 */
bool TagStorage::fulltextSearch(const string &table, const ConditionContainerFullText &cont,
                                int offset, int limit, vector<TagList> &tags) {
    Timer timer;
    // check primary key
    string idCol;
    if(!conn_ || !conn_->primaryKey(table, idCol)) {
        LOGG(Logger::ERROR) << "Table "+table+" doesn't have a primary key" << Logger::FLUSH;
        return false;
    }

    // prepare statement
    Timer inner;
    vector<Vertex::VertexId> ids;
    vector<int> searchProfile = {2, 3, 4, 6, 8, numeric_limits<int>::max()};
    for(int iter : searchProfile) {
        unique_ptr<PrepStmt> stmt;
        if(!buildAndBindFulltextQuery(table, cont, offset, limit, iter, stmt))
            return false;

        vector<Vertex::VertexId> curr;
        while(stmt->step()) {
            Vertex::VertexId id = stmt->column_int64(0);
            curr.push_back(id);
        }

        ids = curr;
        if(curr.size() == limit) {
            break;
        }
    }

    inner.stop();
    LOGG(Logger::INFO) << "[SEARCH]: finding ids" << inner.getElapsedTimeSec() << " sec." << Logger::FLUSH;

    if(ids.size() > 0) {
        // add tags from simple search
        ConditionContainer simple;
        simple.addIdsIn(ids);

        if(!simpleSearch(table, simple, 0, ids.size(), tags))
            return false;

        vector<vector<Vertex::VertexId> > innerIds;
        if(getObjectsInGroups(table, ids, innerIds))
            for(int i = 0; i < tags.size(); i++) {
                tags[i].setObjects(innerIds[i]);
            }
    }

    timer.stop();
    LOGG(Logger::INFO) << "[SEARCH]: total search" << timer.getElapsedTimeSec() << " sec." << Logger::FLUSH;
    return true;
}
/**
 * @brief TagStorage::simpleSearch
 * @param table
 * @param conds
 * @param tags
 * @return
 */
bool TagStorage::simpleSearch(const string &table, ConditionContainer &cont,
                              int offset, int limit, vector<TagList> &tags, bool joinTags) {
    Timer timer;
    tags = vector<TagList>();
    unique_ptr<PrepStmt> stmt;
    if(!buildAndBindQuery(table, cont.getConditions(), offset, limit, stmt))
        return false;

    vector<Vertex::VertexId> ids;
    Vertex::VertexId prev = Vertex::NullVertexId;
    while(stmt->step()) {
        Vertex::VertexId id = stmt->column_int64(0);
        if(ids.size() == 0 || ids[ids.size()-1] != id)
            ids.push_back(id);

        int off = 0;
        if(joinTags) {
            string tagstr = stmt->column_text(1+off);
            if(tagstr == "") {
                LOGG(Logger::WARNING) << "[TAG INDEX] empty tag " << id << Logger::FLUSH;
            }

            string valuestr = stmt->column_text(2+off);
            if(valuestr == "") {
                LOGG(Logger::WARNING) << "[TAG INDEX] empty value " << id << Logger::FLUSH;
            }

            // this assumes that the ids in the output are grouped
            KeyValuePair tag(tagstr, valuestr);
            if(id != prev) {
                tags.push_back(TagList(id));
                prev = id;
            }
            tags[tags.size()-1].add(tag);
        } else {
            tags.push_back(TagList(id));
        }
    }

    // add fixed tags
    vector<vector<KeyValuePair> > fixedTags;
    if(!getFixedTags(table, ids, fixedTags))
        LOGG(Logger::WARNING) << "Couldn't extract fixed tags" << Logger::FLUSH;

    // add objects if the id is a group
    vector<vector<Vertex::VertexId> > innerIds;
    if(getObjectsInGroups(table, ids, innerIds))
        for(int i = 0; i < tags.size(); i++) {
            tags[i].setObjects(innerIds[i]);
        }

    if(fixedTags.size() == tags.size()) {
        for(int i = 0; i < tags.size(); i++) {
            for(int j = 0; j < fixedTags[i].size(); j++) {
                tags[i].add(fixedTags[i][j]);
            }
        }
    } else {
        LOGG(Logger::ERROR) << "The sized of simple and fixed tags don't match" << Logger::FLUSH;
    }

    timer.stop();
    LOGG(Logger::INFO) << "Find tags query: " << timer.getElapsedTimeSec() << " sec." << Logger::FLUSH;
    return true;
}
/**
 * @brief write
 * @param table
 * @param id
 * @param list
 * @return
 */
bool TagStorage::write(const string &table, const TagList &tagList) {
    string idCol;
    if(!conn_ || !conn_->primaryKey(table, idCol)) {
        LOGG(Logger::ERROR) << "Table "+table+" doesn't have a primary key" << Logger::FLUSH;
        return false;
    }

    if(!conn_ || !conn_->existsTable(table)) {
        LOGG(Logger::ERROR) << "Inserting into a nonexisting table" << Logger::FLUSH;
        return false;
    }

    // get a new id if the id is missing
    Vertex::VertexId id = tagList.getId();
    if(id == Vertex::NullVertexId) {
        unique_ptr<PrepStmt> stmt;
        SqlQuery idQ = SqlQuery::q().select({"max(id)"}).from(table);
        if(!conn_->prepare(idQ.toString(), stmt))
            return false;

        if(!stmt->step())
            return false;

        id = stmt->column_int64(0);
        if(id+1 == Vertex::NullVertexId) {
            LOGG(Logger::ERROR) << "the table is full" << Logger::FLUSH;
            return false;
        }
        id++;
    }

    unique_ptr<PrepStmt> stmt;
    string query = "INSERT INTO "+table+" VALUES (?, ?, ?)";
    if(!conn_->prepare(query, stmt))
        return false;

    unique_ptr<PrepStmt> delStmt;
    string delQuery = "DELETE FROM "+table+" WHERE "+idCol+"=? AND tag=? AND value=?";
    if(!conn_->prepare(delQuery, delStmt))
        return false;

    vector<KeyValuePair> tags = tagList.getTags();
    for(const KeyValuePair &pair : tags) {
        // delete the old value
        if(!delStmt->bind(id) || !delStmt->bind(pair.getKey()) || !delStmt->bind(pair.getValue()))
            return false;
        if(!delStmt->exec()) {
            LOGG(Logger::ERROR) << "Not deleted " << pair.getKey() << " " <<
                                   pair.getValue() << " in " << table << Logger::FLUSH;
        }

        // insert the new value
        if(!stmt->bind(id) || !stmt->bind(pair.getKey()) || !stmt->bind(pair.getValue()))
            return false;
        if(!stmt->exec())
            return false;
        delStmt->reset();
        stmt->reset();
    }
    return true;
}
/**
 * @brief getCompressedTags
 * @param table
 * @param ids
 * @param tags
 * @return
 */
bool TagStorage::getFixedTags(const string &table, const vector<Vertex::VertexId> &ids, vector<vector<KeyValuePair> > &tags) {
    Timer timer;
    tags = vector<vector<KeyValuePair> >(ids.size());

    unique_ptr<PrepStmt> stmt;
    if(!buildAndBindFixedQuery(table, ids, stmt))
        return false;

    map<Vertex::VertexId, vector<int> > posMap;
    for(int i = 0; i < ids.size(); i++) {
        if(posMap.count(ids[i])) {
            posMap.find(ids[i])->second.push_back(i);
        } else {
            posMap[ids[i]] = {i};
        }
    }

    while(stmt->step()) {
        Vertex::VertexId id = stmt->column_int64(0);

        string keys = stmt->column_text(1);
        SimpleTokenator tokenator(keys, '=', '\"', false);
        vector<string> tokens = tokenator.getTokens();

        if(tokens.size() == 2) {
            for(int i = 0; i < posMap[id].size(); i++) {
                int pos = posMap[id][i];
                tags[pos].push_back({tokens[0], tokens[1]});
            }
        } else {
            LOGG(Logger::ERROR) << "Invalid fixed tag: " << id << " " << Logger::FLUSH;
        }
    }

    timer.stop();
    LOGG(Logger::INFO) << "[GET FIXED TAGS]: " << timer.getElapsedTimeSec() << Logger::FLUSH;
    return true;
}
/**
 * @brief TagStorage::::getGroupsForObjects
 * @param ids
 * @param groupIds
 * @return
 */
bool TagStorage::getGroupsForObjects(const string &table,
                                     const vector<Vertex::VertexId> &ids,
                                     vector<vector<Vertex::VertexId> > &groupIds) {
    SqlQuery query;
    query = query.select({"objectid", "groupid"}).from(table+"_rel").in("objectid", ids.size());
    LOGG(Logger::DEBUG) << query.toString() << Logger::FLUSH;

    unique_ptr<PrepStmt> stmt;
    if(!conn_ || !conn_->prepare(query.toString(), stmt)) {
        LOGG(Logger::ERROR) << "Couldn't prepare statement " << query.toString() << Logger::FLUSH;
        return false;
    }

    if(!stmt->bind(ids)) {
        LOGG(Logger::ERROR) << "Couldn't bind ids " << query.toString() << Logger::FLUSH;
        return false;
    }

    Vertex::VertexId prev = Vertex::NullVertexId;
    while(stmt->step()) {
        Vertex::VertexId id = stmt->column_int64(0);
        Vertex::VertexId groupId = stmt->column_int64(1);
        if(id != prev) {
            groupIds.push_back(vector<Vertex::VertexId>());
            prev = id;
        }
        groupIds[groupIds.size()-1].push_back(groupId);
    }
    return true;
}
/**
 * @brief TagStorage::getObjectsInGroup
 * @return
 */
bool TagStorage::getObjectsInGroups(const string &table,
                                    const vector<Vertex::VertexId> &groupIds,
                                    vector<vector<Vertex::VertexId> > &ids) {

    string idCol;
    if(!conn_ || !conn_->primaryKey(table, idCol)) {
        LOGG(Logger::ERROR) << "Table "+table+" doesn't have a primary key" << Logger::FLUSH;
        return false;
    }

    SqlQuery query = SqlQuery::q().select({"groupid", "objectid"}).from(table+"_rel").
            in(idCol, groupIds.size()).orderAsc("groupid").orderAsc("objectorder");
    LOGG(Logger::INFO) << query.toString() << Logger::FLUSH;

    unique_ptr<PrepStmt> stmt;
    if(groupIds.size() == 0 || !conn_ || !conn_->prepare(query.toString(), stmt) || !stmt->bind(groupIds))
        return false;

    map<Vertex::VertexId, vector<int> > pos;
    for(int i = 0; i < groupIds.size(); i++) {
        if(pos.count(groupIds[i]))
            pos.find(groupIds[i])->second.push_back(i);
        else
            pos[groupIds[i]] = {i};
    }

    ids = vector<vector<Vertex::VertexId> >(groupIds.size(), vector<Vertex::VertexId>());
    while(stmt->step()) {
        Vertex::VertexId gid = stmt->column_int64(0);
        Vertex::VertexId vid = stmt->column_int64(1);
        for(int i = 0; i < pos[gid].size(); i++)
            ids[pos[gid][i]].push_back(vid);
    }

    return true;
}
/**
 * @brief TagStorage::Initializer::Initializer
 * @param url
 * @param props
 */
TagStorage::Initializer::Initializer(const URL &url, const Properties &props)
    : url_(url), props_(props) {;}
/**
 * @brief TagStorage::Initializer::init
 * @param storage
 * @return
 */
bool TagStorage::Initializer::init(TagStorage &storage) const {
    if(!storage.open(url_, props_)) {
        LOGG(Logger::ERROR) << "Couldnt init tag storage" << Logger::FLUSH;
        storage.close();
        return false;
    }
    return true;
}
/**
 * @brief TagStorage::Destructor::release
 * @param storage
 * @return
 */
bool TagStorage::Destructor::release(TagStorage &storage) {
    if(!storage.close())
        return false;
    return true;
}
//--------------------------------------------------------------------------------------------------
// SqlStream
//--------------------------------------------------------------------------------------------------
/**
 * @brief SqliteStream::SqliteStream
 * @param sqliteFile
 * @param createIfMissing
 */
SqlStream::SqlStream() : currColId_(-1), table_("") {;}
/**
 * @brief SqliteStream::~SqliteStream
 */
SqlStream::~SqlStream() {
    close();
}
/**
 * @brief SqliteStream::prepareTableStream
 * @param tableName
 */
bool SqlStream::open(const URL &url, const Properties &props) {
    if(!close()) {
        return false;
    } else {
        conn_ = ConnectionsManager::getConnection(url, props);
        if(!conn_ || !conn_->open(url, props))
            return false;
        table_ = props.get("table");
        if(table_ != "") {
            string query = "SELECT * FROM "+table_;
            return conn_->prepare(query, stmt_);
        }
        return true;
    }
}
/**
 * @brief SqliteStream::prepareTableStream
 * @param tableName
 */
bool SqlStream::close() {
    // reset non pointer members
    table_ = "";
    currColId_ = -1;

    // try to destruct everything
    bool ret = true;
    if(stmt_ && !stmt_->finalize())
        ret = false;
    stmt_.reset(0);
    if(conn_ && !conn_->close())
        ret = false;
    conn_.reset(0);
    return ret;
}
/**
 * @brief getNumRows
 * @return
 */
int64_t SqlStream::getNumRows() const {
    if(conn_) {
        unique_ptr<PrepStmt> local;
        string query = "SELECT count(*) FROM "+table_;
        if(!conn_->prepare(query, local)) {
            return 0;
        }
        if(local) {
            if(!local->step())
                return 0;
            return local->column_int64(0);
        }
    }
    return 0;
}
/**
 * @brief getNumRows
 * @param table
 * @return
 */
int64_t SqlStream::getNumRows(const string &table) const {
    if(conn_) {
        unique_ptr<PrepStmt> local;
        string query = "SELECT count(*) FROM "+table;
        if(!conn_->prepare(query, local)) {
            return 0;
        }
        if(local) {
            if(!local->step())
                return 0;
            return local->column_int64(0);
        }
    }
    return 0;
}
/**
 * @brief SqliteStream::hasNext
 * @return
 */
bool SqlStream::getNext() {
    return stmt_ && stmt_->step();
}
/**
 * @brief SqliteStream::eof
 * @return
 */
bool SqlStream::eof() {
    return !getNext();
}
/**
 * @brief SqliteStream::operator >>
 * @param var
 * @return
 */
SqlStream &SqlStream::operator >> (int8_t &var) {
    if(stmt_) {
        if(currColId_ == stmt_->column_count()-1) {
            currColId_ = 0;
        } else
            currColId_++;
        var = stmt_->column_int(currColId_);
    }
    return (*this);
}

/**
 * @brief SqliteStream::operator >>
 * @param var
 * @return
 */
SqlStream &SqlStream::operator >> (uint8_t &var) {
    if(stmt_) {
        if(currColId_ == stmt_->column_count()-1) {
            currColId_ = 0;
        } else
            currColId_++;
        var = stmt_->column_int(currColId_);
    }
    return (*this);
}

/**
 * @brief SqliteStream::operator >>
 * @param var
 * @return
 */
SqlStream &SqlStream::operator >> (int64_t &var) {
    if(stmt_) {
        if(currColId_ == stmt_->column_count()-1) {
            currColId_ = 0;
        } else
            currColId_++;
        var = stmt_->column_int64(currColId_);
    }
    return (*this);
}

/**
 * @brief SqliteStream::operator >>
 * @param var
 * @return
 */
SqlStream &SqlStream::operator >> (uint64_t &var) {
    if(stmt_) {
        if(currColId_ == stmt_->column_count()-1) {
            currColId_ = 0;
        } else
            currColId_++;
        var = stmt_->column_int64(currColId_);
    }
    return (*this);
}

/**
 * @brief SqliteStream::operator >>
 * @param var
 * @return
 */
SqlStream &SqlStream::operator >> (int32_t &var) {
    if(stmt_) {
        if(currColId_ == stmt_->column_count()-1) {
            currColId_ = 0;
        } else
            currColId_++;
        var = stmt_->column_int64(currColId_);
    }
    return (*this);
}

/**
 * @brief SqliteStream::operator >>
 * @param var
 * @return
 */
SqlStream &SqlStream::operator >> (uint32_t &var) {
    if(stmt_) {
        if(currColId_ == stmt_->column_count()-1) {
            currColId_ = 0;
        } else
            currColId_++;
        var = stmt_->column_int64(currColId_);
    }
    return (*this);
}

/**
 * @brief SqliteStream::operator >>
 * @param var
 * @return
 */
SqlStream &SqlStream::operator >> (double &var) {
    if(stmt_) {
        if(currColId_ == stmt_->column_count()-1) {
            currColId_ = 0;
        } else
            currColId_++;
        var = stmt_->column_double(currColId_);
    }
    return (*this);
}

/**
 * @brief SqliteStream::operator >>
 * @param var
 * @return
 */
SqlStream &SqlStream::operator >> (float &var) {
    if(stmt_) {
        if(currColId_ == stmt_->column_count()-1) {
            currColId_ = 0;
        } else
            currColId_++;
        var = stmt_->column_double(currColId_);
    }
    return (*this);
}

/**
 * @brief SqliteStream::operator >>
 * @param var
 * @return
 */
SqlStream &SqlStream::operator >> (string &var) {
    if(stmt_) {
        if(currColId_ == stmt_->column_count()-1) {
            currColId_ = 0;
        } else
            currColId_++;
        var = stmt_->column_text(currColId_);
    }
    return (*this);
}
/**
 * @brief open
 * @param url
 * @param props
 * @return
 */
bool TileStorage::open(const URL &url, const Properties &props) {
    conn_ = ConnectionsManager::getConnection(url, props);
    if(!conn_ || !conn_->open(url, props))
        return false;
    // prepare statement for getTile()
    SqlQuery query = SqlQuery::q().select({"tile_data"}).from("tiles").
            where("zoom_level = ? AND tile_column = ? AND tile_row = ?");
    if(!conn_->prepare(query.toString(), stmt_))
        return false;
    return true;
}

/**
 * @brief close
 * @return
 */
bool TileStorage::close() {
    stmt_.reset(0);
    if(conn_ && !conn_->close())
        return false;
    conn_.reset(0);
    return true;
}

/**
 * @brief getTile
 * @param zoom
 * @param column
 * @param row
 */
string TileStorage::getTile(const string& zoom, const string& column, const string& row) const {
    if(!stmt_ || !stmt_->reset())
        return "";
    if(!stmt_->bind(zoom) || !stmt_->bind(column) || !stmt_->bind(row))
        return "";
    if(!stmt_->step())
        return "";
    return stmt_->column_blob(0);
}

AuxiliarySpatialStorage::AuxiliarySpatialStorage() {
    ;
}

bool AuxiliarySpatialStorage::open(const URL &url, const Properties &props) {
    if(!close()) {
        return false;
    } else {
        string geomTable = props.get("geometry_table");// water_polygons
        if(geomTable == "") {
            LOGG(Logger::ERROR) << "[AuxiliaryIndexSql] No geometry table name provided" << Logger::FLUSH;
            return false;
        }

        string geomColId = props.get("geometry_id_column");//"PK_UID";
        if(geomColId == "") {
            LOGG(Logger::ERROR) << "[AuxiliaryIndexSql] No geometry id column name provided" << Logger::FLUSH;
            return false;
        }

        string geomColumn = props.get("geometry_column");// "Geometry";
        if(geomColumn == "") {
            LOGG(Logger::ERROR) << "[AuxiliaryIndexSql] No geometry column name provided" << Logger::FLUSH;
            return false;
        }

        string indexTable = props.get("index_table");// "idx_water_polygons_Geometry";
        if(indexTable == "") {
            LOGG(Logger::ERROR) << "[AuxiliaryIndexSql] No geometry index table name provided" << Logger::FLUSH;
            return false;
        }
        string indexColId = props.get("index_id_column");// "pkid";
        if(indexColId == "") {
            LOGG(Logger::ERROR) << "[AuxiliaryIndexSql] No index column id name provided" << Logger::FLUSH;
            return false;
        }

        string xMin = props.get("index_xmin_column");
        if(xMin == "") {
            LOGG(Logger::ERROR) << "[AuxiliaryIndexSql] No index xmin column name provided" << Logger::FLUSH;
            return false;
        }
        string xMax = props.get("index_xmax_column");
        if(xMax == "") {
            LOGG(Logger::ERROR) << "[AuxiliaryIndexSql] No index xmax column name provided" << Logger::FLUSH;
            return false;
        }

        string yMin = props.get("index_ymin_column");
        if(yMin == "") {
            LOGG(Logger::ERROR) << "[AuxiliaryIndexSql] No index ymin column name provided" << Logger::FLUSH;
            return false;
        }

        string yMax = props.get("index_ymax_column");
        if(yMax == "") {
            LOGG(Logger::ERROR) << "[AuxiliaryIndexSql] No index ymax column name provided" << Logger::FLUSH;
            return false;
        }

        conn_ = ConnectionsManager::getConnection(url, props);
        if(!conn_ || !conn_->open(url, props)) {
            LOGG(Logger::ERROR) << "Failed to open database" << url.getPath() << Logger::FLUSH;
            return false;
        }

        SqlQuery inner = SqlQuery::q().select({indexTable + "." + indexColId})
                .from(indexTable).where(xMin + ">=? AND " + xMax + "<=? AND " + yMin + ">=? AND "+ yMax + "<=?");

        vector<string> tables = {indexTable, geomTable};
        SqlQuery nearestPoly = SqlQuery::q().select({geomTable + "." + geomColId, geomTable + "." + geomColumn})
                .from(tables).in(indexTable + "." + indexColId, inner)
                .where(indexTable + "." + indexColId + "=" + geomTable + "." + geomColId);
        if(!conn_->prepare(nearestPoly.toString(), selectStmt_)) {
            LOGG(Logger::ERROR) << "Failed to prepare select stmt" << nearestPoly.toString() << Logger::FLUSH;
            return false;
        }
    }
    return true;
}

bool AuxiliarySpatialStorage::close() {
    bool ret = true;
    if(selectStmt_ && !selectStmt_->finalize())
        ret = false;
    selectStmt_.reset(0);
    if(conn_ && !conn_->close())
        ret = false;
    conn_.reset(0);
    return ret;
}

bool AuxiliarySpatialStorage::findGeometry(const Point &hiLeft, const Point &lowRight, std::vector<string> &geometries) {
    if(!selectStmt_->bind(hiLeft.lon()) || !selectStmt_->bind(lowRight.lon())) {
        LOGG(Logger::ERROR) << "Failed to bind select stmt" << Logger::FLUSH;
        return false;      
    }
    if(!selectStmt_->bind(lowRight.lat()) || !selectStmt_->bind(hiLeft.lat())) {
        LOGG(Logger::ERROR) << "Failed to bind select stmt" << Logger::FLUSH;
        return false;
    }
    // TODO make different types of geometry format
    // now we read blob(WKB), might be WKT
    while(selectStmt_->step()) {
        // extract id
        //Vertex::VertexId id = selectStmt_->column_int64(0);
        string pData = selectStmt_->column_blob(1);
        geometries.emplace_back(pData);
    }
    return true;
}

AuxiliarySpatialStorage::~AuxiliarySpatialStorage() {
    close();
}
