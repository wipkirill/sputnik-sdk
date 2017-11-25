#include "TilePlugin.h"
#include "GeometryRoutines.h"

#include <UrbanLabs/Sdk/Storage/SqlConsts.h>
#include <UrbanLabs/Sdk/Config/XmlConfig.h>
#include <UrbanLabs/Sdk/Utils/FileSystemUtil.h>
#include <UrbanLabs/Sdk/Storage/Storage.h>

#include <sstream>

using namespace std;
using namespace tools;

size_t TileLayer::totalPoints_ = 0;
size_t TileLayer::totalLines_ = 0;
size_t TileLayer::totalPolygons_ = 0;
TileLayer::StringSet TileLayer::nonSplittable_ = set<string>();
TileLayer::StyleGroupMap TileLayer::styleGroup_ = std::unordered_map<std::string,std::string>{};

/**
 * @brief TileLayer::TileLayer
 * @param baseFileName
 * @param name
 * @param lType
 * @param kv
 * @param tableFields
 */
TileLayer::TileLayer(const string &baseFileName, const string &name,
             GeometryType lType, const AssocArray &kv,
             const StringSet &tableFields, const ComputedFieldMap &computedFields, bool compress)
    : name_(name), layerDesc_(0),
      type_(lType), tableStmt_(), baseFileName_(baseFileName),
      tableFields_(tableFields), computedFields_(computedFields),vpIndex_(), kv_(kv),
      compression_(compress)
{
    URL url(baseFileName_+".vp.sqlite");
    Properties props = {{"type", "sqlite"},{"table", SqlConsts::VP_TABLE}};
/*        Properties props = {{"type", "sqlite"},{"table", SqlConsts::VP_TABLE}, 
    {"auxUrl", "world.vp.sqlite"}, {"auxType", "sqlite"},
    {"auxTable", SqlConsts::VP_TABLE}};*/

    if(!vpIndex_.open(url, props)) {
        LOGG(Logger::ERROR) << "can't initialize vp index" << Logger::FLUSH;
        exit(EXIT_FAILURE);
    }
    string layerFname = baseFileName_+"."+getName();
    ifstream file(layerFname.c_str());
    if (file) {
        die("TILE PLUGIN","Tile data file already exists for "+getName());
    }
    layerDesc_ = fopen(layerFname.c_str(), "a");
}
/**
 * @brief getName
 * @return
 */
string TileLayer::getName() const {
    return name_;
}
/**
 * @brief TileLayer::getType
 * @return
 */
TileLayer::GeometryType TileLayer::getType() const {
    return type_;
}
/**
 * @brief getTableStmt
 * @return
 */
string TileLayer::getTableStmt() const {
    string tableStmt = "CREATE TABLE '"+name_+"' (OGC_FID INTEGER PRIMARY KEY, 'GEOMETRY' BLOB ";
    for (const string& field : tableFields_) {
        tableStmt += ",'"+field+"' VARCHAR";
    }
    // first element in set<string> is field name and second is function
    for(const pair<string,ComputedField > &entry : computedFields_) {
        string funcName = getFuncName(entry.second);
        tableStmt += ",'"+entry.first+"' "+getFieldType(funcName);
    }

    tableStmt.append(");");
    return tableStmt;
}
/**
 * @brief TileLayer::getFuncData
 * @param data
 * @return
 */
string TileLayer::getFuncName(const ComputedField &data) const {
    if(data.count("function") == 0)
        assert(false);
    return data.find("function")->second;
}
/**
 * @brief TileLayer::getObjTag
 * @param data
 * @return
 */
string TileLayer::getObjTag(const ComputedField &data) const {
    if(data.count("objectTag") == 0)
        return "";
    return data.find("objectTag")->second;
}
/**
 * @brief TileLayer::getFieldType
 * @return
 */
string TileLayer::getFieldType(const string &fieldFunction) const {
    if(fieldFunction == "styleGroupFunction")
        return "VARCHAR";
    if(fieldFunction == "areaFunction")
        return "REAL";
    assert(false);
    return "";
}
/**
 * @brief TileLayer::matchStyleGroup
 * @param tag
 * @return
 */
string TileLayer::matchStyleGroup(const string &tag) const {
    if(styleGroup_.count(tag) > 0)
        return styleGroup_.find(tag)->second;
    return "other";
}
/**
 * @brief TileLayer::getIndexStmt
 * @return
 */
string TileLayer::getIndexStmt() const {
    string indexStmt = "CREATE VIRTUAL TABLE " + name_+"_index"
                       " using rtree(pkid, xmin, xmax, ymin, ymax);";
    return indexStmt;
}
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
void TileLayer::makeSqlInsert(Vertex::VertexId id, const T* object, const G &geometry, const string &data, string &stmt) {
    std::stringstream s;
    s << "INSERT INTO " << name_ << " VALUES(" << lexical_cast(id) <<
            "," << "X'" << data << "'";

    for (const string& field : tableFields_) {
        string val = extractTagValue<T>(field,object);
        if (val.empty())
            s << ", NULL";
        else
            s <<  ",'" << val << "'";
    }

    // first element in set<string> is field name and second is function
    for(const auto &entry : computedFields_) {
        string objField = getObjTag(entry.second);
        string funcName = getFuncName(entry.second);
        string argument = extractTagValue<T>(objField,object);
        s << ",'" << computeFunctionVal(geometry, funcName, argument) << "'";
    }

    s << ");";
    stmt = s.str();
}
/**
 *
 */
template <typename T>
void TileLayer::getBboxStmt(Vertex::VertexId id, const T &geometry, string &indexStmt) {
    vector<Point> bbox;
    bbox = geometry.getBoundingBox(true);
    indexStmt = "INSERT INTO "+name_+"_index VALUES("+lexical_cast(id)+
            ","+lexical_cast(bbox[0].lon())+","+lexical_cast(bbox[1].lon())+
            ","+lexical_cast(bbox[0].lat())+","+lexical_cast(bbox[1].lat())+
            ");";
}
/**
 * @brief inProps
 * @param key
 * @param props
 * @return
 */
bool TileLayer::inProps(const string &key, set<string> &props) {
    if (props.find("*") != props.end())
        return true;
    return props.find(key) != props.end();
}
/**
 * @brief TileLayer::serializeFeature
 * @param object
 * @param serialized
 * @return
 */
bool TileLayer::serializeFeature(OSMNode* object, string &serialized, string &indexStmt) {
    return serializePoint(object, serialized, indexStmt);
}
/**
 * @brief TileLayer::serializeFeature
 * @param object
 * @param serialized
 * @return
 */
bool TileLayer::serializeFeature(OSMWay *object, string &serialized, string &indexStmt) {
    if (type_ == LINE)
        return serializeLine(object, serialized, indexStmt);
    if (type_ == POLYGON)
        return serializePolygon(object, serialized, indexStmt);
    return false;
}
/**
 * @brief TileLayer::serializePoint
 * @param object
 * @param serialized
 * @return
 */
bool TileLayer::serializePoint(const OSMNode *object, string &serialized, string &indexStmt) {
    totalPoints_++;
    Point pt(object->dfLat,object->dfLon);
    getBboxStmt(totalPoints_, pt, indexStmt);

    string res = applyCompression<Point>(pt, StoredType::POINT);
    makeSqlInsert<OSMNode>(totalPoints_, object, pt, res, serialized);
    return true;
}
/**
 * @brief TileLayer::getSkip
 * @param way
 * @return
 */
int TileLayer::getSkip(const OSMWay* way) const {
    if(way->nRefs > 40)
        return 20;
    return numeric_limits<int>::max();
}
/**
 * @brief serializeLine
 * @param way
 * @param serialized
 * @return
 */
bool TileLayer::serializeLine(const OSMWay* way, string &serialized, string &indexStmt) {
    vector<VertexId> vid;
    for(int i = 0; i < (int)way->nRefs; ++i) {
        vid.push_back(way->nodeRefs[i]);
    }

    vector<int8_t> found;
    vector<Point> pts, curr;
    vpIndex_.getPoints(vid, pts, found);

    for(size_t i = 0; i < way->nRefs; ++i) {
        if(found[i]) {
            curr.push_back(pts[i]);
        }
        if((!found[i] || i+1 == way->nRefs) && curr.size() > 0) {
            int skip = getSkip(way);
            if(isNonSplittable(way)) {
                skip = numeric_limits<int>::max();
            }

            if(curr.size() >= 2) {
                //simplifyPreserveTopologyLine(curr, 1e-4);
                vector<vector<Point> > segs = splitLine(curr, skip);
                for(size_t j = 0; j < segs.size(); j++) {
                    LineString lineStr;
                    for(size_t k = 0; k < segs[j].size(); k++) {
                        lineStr.addPoint(segs[j][k]);
                    }

                    // generate the index statment
                    string index;
                    getBboxStmt<LineString>(totalLines_, lineStr, index);
                    indexStmt.append(index+"\n");

                    // generate the way data
                    string stmt;
                    string res = applyCompression<LineString>(lineStr, StoredType::LINE);
                    makeSqlInsert<OSMWay>(totalLines_, way, lineStr, res, stmt);
                    serialized.append(stmt+"\n");
                    totalLines_++;
                }
            }
            curr.clear();
        }
    }

    return true;
}
/**
 * @brief serializePolygon
 * @param way
 * @param serialized
 * @return
 */
bool TileLayer::serializePolygon(const OSMWay* way, string &serialized, string &indexStmt) {
    vector<VertexId> vid;
    for(size_t i = 0; i < way->nRefs; ++i) {
        vid.push_back(way->nodeRefs[i]);
    }

    vector<Point> pts;
    vector<int8_t> found;
    vpIndex_.getPoints(vid, pts, found);
    // TODO make a resolver in case of missing points in PBF
    fixPolygon(found, pts);

    vector<vector<vector<Point> > > rings;
    buildPolygons({pts}, rings, 0);

    if(rings.size() == 0) {
        LOGG(Logger::WARNING) << "Failed to build polygon in serializePolygon" << way->nID << Logger::FLUSH;
        return false;
    }
    // store each of the rings
    for(size_t i = 0; i < rings.size(); ++i) {
        Polygon pg;
        pts = rings[i][0];
        pg.addRing(pts);

        getBboxStmt(totalPolygons_, pg, indexStmt);

        string res = applyCompression<Polygon>(pg, StoredType::POLYGON);
        makeSqlInsert<OSMWay>(totalPolygons_, way, pg, res, serialized);
        totalPolygons_++;
    }

    return true;
}
/**
 * @brief TileLayer::processRelation
 * @param rel
 * @param waysToNodesMap
 * @return
 */
bool TileLayer::processRelation(OSMRelation *rel, const WayNodesMap &waysToNodesMap) {
    vector<VertexId> outerWays, innerWays;

    // collect outer and inner ways
    for(size_t j = 0; j < rel->nMembers; ++j) {
        if(rel->members[j].eType == OSMMemberType::MEMBER_WAY) {
           if(rel->members[j].role == "outer")
               outerWays.push_back(rel->members[j].nID);
           if(rel->members[j].role == "inner")
               innerWays.push_back(rel->members[j].nID);
        }
    }
    if(outerWays.size() == 0) {
        LOGG(Logger::ERROR) << "No outer ways in multipolygon " << rel->nID << Logger::FLUSH;
        return false;
    }
    // collect wayIds for each outer
    vector<vector<VertexId> > outRings;
    for(auto wayId : outerWays) {
        if(waysToNodesMap.count(wayId) > 0) {
            outRings.push_back(waysToNodesMap.find(wayId)->second);
        } else {
            LOGG(Logger::ERROR) << "No value" << wayId << "in waysToNodesMap for outerWays" << Logger::FLUSH;
            return false;
        }
    }
    // collect wayIds for each inner
    vector<vector<VertexId> > inRings;
    for(auto wayId : innerWays) {
        if(waysToNodesMap.count(wayId) > 0) {
            inRings.push_back(waysToNodesMap.find(wayId)->second);
        } else {
            LOGG(Logger::ERROR) << "No value" << wayId << "in waysToNodesMap for innerWays" << Logger::FLUSH;
            return false;
        }
    }

    vector<vector<Point> > parts;
    // extract Points for each ring and add to polygonRings
    for(size_t i = 0; i < outRings.size(); ++i) {
        vector<int8_t> found;
        vector<Point> outerLRing;
        if(!vpIndex_.getPoints(outRings[i], outerLRing, found)) {
            LOGG(Logger::ERROR) << "Couldn't find some points in index for outer" << Logger::FLUSH;
            return false;
        }
        parts.push_back(outerLRing);
    }

    for(size_t i = 0; i < inRings.size(); ++i) {
        vector<int8_t> found;
        vector<Point> inLRing;
        if(!vpIndex_.getPoints(inRings[i], inLRing, found)) {
            LOGG(Logger::ERROR) << "Couldn't find some points in index for inner" << Logger::FLUSH;
            return false;
        }
        parts.push_back(inLRing);
    }

    vector<vector<vector<Point> > > rings;
    buildPolygons(parts, rings, 0);

    if(rings.size() == 0) {
        LOGG(Logger::ERROR) << "Failed to build a multipolygon" << rel->nID << Logger::FLUSH;
        return false;
    }

    // store each polygon
    for(size_t i = 0; i < rings.size(); ++i) {
        Polygon pg;
        for(size_t j = 0; j < rings[i].size(); ++j)
            pg.addRing(rings[i][j]);

        string data = applyCompression(pg, StoredType::POLYGON);

        string serialized, indexStmt;
        getBboxStmt(totalPolygons_, pg, indexStmt);
        makeSqlInsert(totalPolygons_, rel, pg, data, serialized);
        writeGeomData(serialized, indexStmt);

        totalPolygons_++;
    }

    return true;
}
/**
*
*/
void TileLayer::writeGeomData(const std::string &geomCreateSql, const std::string &indexSql) const {
    std::string printSpec = type_spec<std::string>()+"\n";
    fprintf(layerDesc_, printSpec.c_str(), geomCreateSql.c_str());
    fprintf(layerDesc_, printSpec.c_str(), indexSql.c_str());
}

/**
 * @brief TilePlugin::TilePlugin
 * @param outputFile
 */
TilePlugin::TilePlugin(const string& outputFile, const string &configFile, bool /*compress*/):  currPassId_(0),
    seenWays_(), outputFileName_(), layers_(),relLayers_(),waysFromMultipolygons_(), waysToNodesMap_()
{    
    outputFileName_ = outputFile;
    pluginId_ = "tilePlugin";
    XmlConfig xConf;
    if(!xConf.init(configFile,pluginId_))
        die(pluginId_,"Failed to read config file");

    if(!xConf.getMap("styleGroup", TileLayer::styleGroup_))
        die(pluginId_,"Failed to read styleGroup from config file");

    if(!xConf.getStringSet("nonSplittable", TileLayer::nonSplittable_))
        die(pluginId_,"Failed to read nonSplittable from config file");

    readLayers("layers", xConf, layers_);
    readLayers("relationLayers", xConf, relLayers_);
    waysFromMultipolygons_.set_deleted_key(Vertex::NullVertexId);
    waysToNodesMap_.set_deleted_key(Vertex::NullVertexId);
    seenWays_.set_deleted_key(Vertex::NullVertexId);

    URL url(outputFileName_+".vp.sqlite");
    Properties props = {{"type", "sqlite"},{"table", SqlConsts::VP_TABLE}};
/*        Properties props = {{"type", "sqlite"},{"table", SqlConsts::VP_TABLE}, 
    {"auxUrl", "world.vp.sqlite"}, {"auxType", "sqlite"},
    {"auxTable", SqlConsts::VP_TABLE}};*/

    if(!vpIndex_.open(url, props)) {
        LOGG(Logger::ERROR) << "can't initialize vp index" << Logger::FLUSH;
        exit(EXIT_FAILURE);
    }
}
/**
 * @brief TilePlugin::readLayers
 * @param propName
 * @param layerList
 */
void TilePlugin::readLayers(const string &propName, XmlConfig &xConf, vector<TileLayer*> &layerList) {
    int count = 0;
    xConf.getObjectCount(propName,count);
    for(int i=0; i < count; ++i) {
        XmlConfig layerConf;
        if(!xConf.getObjectAt(propName,i,layerConf)) {
            die(pluginId_, "Failed to read layer from config");
        }
        string lName;
        if(!layerConf.getString("layerName", lName))
           die(pluginId_, "Failed to read layer name from config");

        bool lCompress;
        if(!layerConf.getBoolean("compress", lCompress))
           die(pluginId_, "Failed to read layer compress property from config");

        string lType;
        TileLayer::GeometryType type;
        if(!layerConf.getString("layerType", lType))
           die(pluginId_, "Failed to read layer type from config");
        if(lType == "point")
            type = TileLayer::POINT;
        else if(lType == "polygon")
            type = TileLayer::POLYGON;
        else if(lType == "line")
            type = TileLayer::LINE;
        else
            die(pluginId_, "Unknown layer type "+lType);
        TileLayer::StringSet storeFields;
        if(!layerConf.getStringSet("storeFields", storeFields))
            die(pluginId_, "Failed to get storeFields from layer "+lName);

        TileLayer::AssocArray tagValues;
        if(!layerConf.getAssocArray("tagsWithValues",tagValues))
            die(pluginId_, "Failed to get tagsWithValues from layer "+lName);

        TileLayer::ComputedFieldMap computedFields;
        readComputedFields(lName, layerConf, computedFields);

        layerList.push_back(new TileLayer(outputFileName_,
                                    lName,
                                    type,
                                    tagValues,
                                    storeFields,
                                    computedFields,
                                    lCompress));
    }
}
/**
 * @brief TilePlugin::readComputedFields
 * @param lName
 * @param conf
 * @param map
 */
void TilePlugin::readComputedFields(const string &lName, XmlConfig &conf, TileLayer::ComputedFieldMap &map) const {
    string fieldName = "computedFields";
    if(conf.hasProperty(fieldName, "objects")) {
        int count = 0;
        if(!conf.getObjectCount(fieldName, count))
            die(pluginId_, "Failed to get object count in computed fields "+lName);
        for(int i=0; i < count; ++i) {
            XmlConfig compFieldConf;
            if(!conf.getObjectAt(fieldName, i, compFieldConf))
                die(pluginId_, "Failed to get object in computed fields "+lName);
            TileLayer::ComputedField fieldData;
            if(!compFieldConf.getMap("fieldConfig", fieldData))
                die(pluginId_, "Failed to read fieldConfig in computed fields "+lName);
            if(fieldData.count("fieldName") == 0)
                die(pluginId_, "Failed to read fieldName in computed fields "+lName);
            map.insert({fieldData["fieldName"], fieldData});
        }
    }
}
/**
 * @brief init
 * @param outputFile
 */
void TilePlugin::init() {
    initTileLayerFiles();
}
/**
 * @brief TilePlugin::getPassNumber
 * @return
 */
int TilePlugin::getPassNumber() {
    return 2;
}
/**
 * @brief TilePlugin::notifyPassNumber
 * @param currPassId
 */
void TilePlugin::notifyPassNumber(const int currPassId) {
    currPassId_ = currPassId;
}
/**
 * @brief node processes one osm node
 * @param node
 */
void TilePlugin::notifyNode(OSMNode* node){
    // nodes need only one pass
    if(currPassId_ == 1)
        for (int i = 0; i < (int)layers_.size();++i) {
            if (layers_[i]->getType() == TileLayer::POINT)
                if (layers_[i]->isInterested<OSMNode>(node)) {
                    if (!layers_[i]->storeFeature(node))
                        die(pluginId_, "Failed to store point feature for "+layers_[i]->getName());
                    break;
                }
        }
}
/**
 * @brief way
 * @param way
 */
void TilePlugin::notifyWay(OSMWay* way) {
    // process ways normally on first pass. See relation pass for next steps
    if (currPassId_ == 1) {
        seenWays_.insert(way->nID);
        for (int i = 0; i < (int)layers_.size();++i) {
            if (layers_[i]->getType() == TileLayer::LINE || layers_[i]->getType() == TileLayer::POLYGON)
                if (layers_[i]->isInterested<OSMWay>(way)) {
                    if (!layers_[i]->storeFeature(way)) {
                        string msg = " Failed to store line/polygon feature for "+layers_[i]->getName();
                        LOGG(Logger::DEBUG) << msg << Logger::FLUSH;
                    }
                    break;
                }
        }
    }
    else {
        // waysFromMultipolygons_ are filled in after first relation pass
        // now store their nodes
        if(waysFromMultipolygons_.count(way->nID) > 0) {
            vector<VertexId> wayNodes(way->nRefs,0);
            for(int k=0;k<(int)way->nRefs;++k)
                wayNodes[k] = way->nodeRefs[k];
            if(wayNodes.size() > 1)
                waysToNodesMap_.insert({way->nID, wayNodes});
            else
               LOGG(Logger::ERROR) << "Way has one element "<<way->nID << Logger::FLUSH;
        }
    }
}
/**
 * @brief TilePlugin::isValidWay
 * @param wayId
 * @return
 */
bool TilePlugin::isValidWay(EdgeId wayId) {
    return seenWays_.count(wayId) > 0;
}
/**
 * @brief TilePlugin::notifyRelation
 * @param rel
 */
void TilePlugin::notifyRelation(OSMRelation* rel) {
    // during first pass
    if(currPassId_ == 1) {
        for(int i=0;i<(int)relLayers_.size();++i) {
            if(relLayers_[i]->isInterested<OSMRelation>(rel)) {
                for(int j=0;j<(int)rel->nMembers;++j) {
                    if(isValidWay(rel->members[j].nID) &&
                       rel->members[j].eType == OSMMemberType::MEMBER_WAY) {
                        waysFromMultipolygons_.insert(rel->members[j].nID);
                    }
                }
            }
        }
    } else {
        for(int i=0;i<(int)relLayers_.size();++i)
            if(relLayers_[i]->isInterested<OSMRelation>(rel)) {
                // validate all relation members
                for(int t=0;t<(int)rel->nMembers;++t)
                    if(!isValidWay(rel->members[t].nID))
                        return;
                relLayers_[i]->processRelation(rel, waysToNodesMap_);
                break;
            }
    }
}
/**
 * @brief getFileNamesToRead is used to .read files from shell
 * this was done because sometimes we need to import BLOB
 * data which cannot be .imported from a CSV file
 * @return
 */
vector<string> TilePlugin::getFileNamesToRead() const {
    vector<string> readFiles;
    for (int i = 0; i < (int)layers_.size();++i) {
        readFiles.push_back(outputFileName_+"."+layers_[i]->getName());
    }
    for (int i = 0; i < (int)relLayers_.size();++i) {
        readFiles.push_back(outputFileName_+"."+relLayers_[i]->getName());
    }
    return readFiles;
}
/**
 * @brief TilePlugin::notifyEndParsing
 */
void TilePlugin::notifyEndParsing() {
    for (int i = 0; i < (int)relLayers_.size();++i) {
        relLayers_[i]->close();
    }
}
/**
*/
void TilePlugin::storeWorldWater() {
    TileLayer *pgWater = nullptr;
    for (int i = 0; i < (int)layers_.size();++i) {
        if(layers_[i]->getName() == "pg_world_water") {
            pgWater = layers_[i];
            break;
        } 
    }
    // check if such layer exists
    if(pgWater == nullptr) {
        LOGG(Logger::ERROR) << "Pg_Water layer not found, not storing world water" << Logger::FLUSH;
        return;
    }  
    string worldWaterFile = worldWaterPath_;
    if(!FsUtil::fileExists(worldWaterFile)) {
        LOGG(Logger::ERROR) << "No world_water database found in config/data folder, not storing world water" << Logger::FLUSH;
        return;
    }
    // settings to query a spatilite database of all water polygons
    URL url(worldWaterFile);
    Properties props = {{"type", "sqlite"}, 
                        {"geometry_table", "water_polygons"},
                        {"geometry_id_column", "PK_UID"},
                        {"geometry_column", "Geometry"},
                        {"index_table", "idx_water_polygons_Geometry"},
                        {"index_id_column", "pkid"},
                        {"index_xmin_column", "xmin"},
                        {"index_xmax_column", "xmax"},
                        {"index_ymin_column", "ymin"}, 
                        {"index_ymax_column", "ymax"}};
    AuxiliarySpatialStorage worldWaterIndex;
    if(!worldWaterIndex.open(url, props)) {
        LOGG(Logger::ERROR) << "Failed to open world water index, not storing world water" << Logger::FLUSH;
        return;
    }
    // get bbox of current map
    vector<Point> bbox;
    if (!vpIndex_.getBoundingBox(bbox)) {
        LOGG(Logger::ERROR) << "Can't find bounding box, not storing world water" << Logger::FLUSH;
        return;
    }
    
    Point hiLeft = bbox[0], lowRight = bbox[1];
    LOGG(Logger::DEBUG) << "Bbox" << hiLeft << lowRight << Logger::FLUSH;
    vector<string> waterGeoms;
    worldWaterIndex.findGeometry(hiLeft, lowRight, waterGeoms);
    std::vector<std::vector<std::vector<Point> > > rings;
    // query only bounding box of the current map
    vector<Point> mapPolygon = {hiLeft, Point(hiLeft.lat(), lowRight.lon()),
                                lowRight, Point(lowRight.lat(), hiLeft.lon()), hiLeft};
    intersect(mapPolygon, waterGeoms, rings);

    if(rings.size() == 0) {
        LOGG(Logger::ERROR) << "No water polygons added" << Logger::FLUSH;
        return;
    } else {
        LOGG(Logger::INFO) << "Stored" << rings.size() << "water polygons" << Logger::FLUSH;
    }

    // store each polygon
    for(size_t i = 0; i < rings.size(); ++i) {
        Polygon pg;
        for(size_t j = 0; j < rings[i].size(); ++j)
            pg.addRing(rings[i][j]);

        string data = pgWater->applyCompression(pg, StoredType::POLYGON);

        string serialized, indexStmt;
        pgWater->getBboxStmt(TileLayer::totalPolygons_, pg, indexStmt);
        OSMWay way;
        pgWater->makeSqlInsert(TileLayer::totalPolygons_, &way, pg, data, serialized);
        pgWater->writeGeomData(serialized, indexStmt);

        TileLayer::totalPolygons_++;
    }
}
/**
 * @brief TilePlugin::finalize
 */
void TilePlugin::finalize() {
    storeWorldWater();
    for (int i = 0; i < (int)layers_.size();++i) {
        layers_[i]->close();
    }
}
/**
 * @brief TilePlugin::cleanUp
 */
void TilePlugin::cleanUp() {
    for (int i = 0; i < (int)layers_.size();++i) {
        string layerFname = outputFileName_+"."+layers_[i]->getName();
        remove(layerFname.c_str());
    }
    for (int i = 0; i < (int)relLayers_.size();++i) {
        string layerFname = outputFileName_+"."+relLayers_[i]->getName();
        std::remove(layerFname.c_str());
    }
}
/**
 * @brief initLayerFiles
 */
void TilePlugin::initTileLayerFiles() {
    string printSpec = type_spec<string>()+"\n";
    for (int i = 0; i < (int)layers_.size();++i) {
        string layerFname = outputFileName_+"."+layers_[i]->getName();
        FILE * layerDesc = fopen(layerFname.c_str(), "a");
        fprintf(layerDesc, printSpec.c_str(), layers_[i]->getTableStmt().c_str());
        fprintf(layerDesc, printSpec.c_str(), layers_[i]->getIndexStmt().c_str());
        fclose(layerDesc);
    }
    for (int i = 0; i < (int)relLayers_.size();++i) {
        string layerFname = outputFileName_+"."+relLayers_[i]->getName();
        FILE * layerDesc = fopen(layerFname.c_str(), "a");
        fprintf(layerDesc, printSpec.c_str(), relLayers_[i]->getTableStmt().c_str());
        fprintf(layerDesc, printSpec.c_str(), relLayers_[i]->getIndexStmt().c_str());
        fclose(layerDesc);
    }
}

void TilePlugin::setWorldWaterPath(const std::string& path) {
    worldWaterPath_ = path;
}
