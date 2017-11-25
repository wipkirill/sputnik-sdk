#include <set>
#include <UrbanLabs/Sdk/Utils/URL.h>
#include <UrbanLabs/Sdk/Utils/Properties.h>
#include <UrbanLabs/Sdk/GraphCore/Point.h>
#include <UrbanLabs/Sdk/GraphCore/Vertices.h>
#include <UrbanLabs/Sdk/Storage/ConnectionsManager.h>
#include "AddressDecoderPlugin.h"
#include <UrbanLabs/Sdk/Storage/SqlConsts.h>
#include "KdTreeSpatial.h"

using namespace std;

/**
 * @brief AddressDecoderPlugin::toLower
 * @param str
 * @return
 */
string AddressDecoderPlugin::toLower(const string &str) {
    if(dbConn_ && toLower_) {
        toLower_->reset();
        if(toLower_->bind(str))
            if(toLower_->step()) {
                string low = toLower_->column_text(0);
                return low;
            }
        return str;
    }
    return str;
}
/**
 * @brief AddressDecoderPlugin::toUpper
 * @param str
 * @return
 */
string AddressDecoderPlugin::toUpper(const string &str) {
    if(dbConn_ && toUpper_) {
        toUpper_->reset();
        if(toUpper_->bind(str))
            if(toUpper_->step()) {
                string up = toUpper_->column_text(0);
                return up;
            }
        return str;
    }
    return str;
}
/**
 * @brief AddressDecoderPlugin::AddressDecoderPlugin
 * @param outFile
 */
AddressDecoderPlugin::AddressDecoderPlugin(const string &outFile) : outputFileName_(outFile) {
    vertexPointFileName_ = outputFileName_+".vp.sqlite";
    areasOutFileName_ = outputFileName_+"."+SqlConsts::ADDRESS_DECODER_AREAS_TABLE;
}
/**
 * @brief AddressDecoderPlugin::init
 */
void AddressDecoderPlugin::init() {
    // initialize vertex to point index
    URL url(vertexPointFileName_);
    Properties props = {{"type", "sqlite"},{"table", SqlConsts::VP_TABLE}};
    if(!vptIndex_.open(url, props))
        die(pluginId_, "can't initialize vertexPoint database");

    // initialize translation
    translateDbName_ = "addressdecode_translate";
    URL transUrl(translateDbName_);
    Properties transProps = {{"type", "sqlite"},{"create", "1"}, {"table", "translate"}};
    dbConn_ = ConnectionsManager::getConnection(transUrl, transProps);
    if(!dbConn_->open(transUrl, transProps)) {
        LOGG(Logger::ERROR) << "Couldn't open database" << Logger::FLUSH;
        return;
    }

    SqlQuery toLowerQ = SqlQuery::q().select({"lower(?)"});
    if(!dbConn_->prepare(toLowerQ.toString(), toLower_)) {
        LOGG(Logger::ERROR) << "Couldn't prepare statement" << Logger::FLUSH;
        return;
    }


    SqlQuery toUpperQ = SqlQuery::q().select({"upper(?)"});
    if(!dbConn_->prepare(toUpperQ.toString(), toUpper_)) {
        LOGG(Logger::ERROR) << "Couldn't prepare statement" << Logger::FLUSH;
        return;
    }

    // initialize output file
    addrAreasFile_ = fopen(areasOutFileName_.c_str(), "w");
    if(!addrAreasFile_) {
        LOGG(Logger::ERROR) << "Couldn't open file address areas" << Logger::FLUSH;
    }
}
/**
 * @brief removeAddr
 * @param s
 * @return
 */
string removeAddrTag(string s) {
    string pre = "addr:";
    if(s.find(pre) != string::npos) {
        s = s.substr(pre.size());
    }
    return s;
}
/**
 * @brief addAddressArea
 */
void AddressDecoderPlugin::addAddressArea(const string &type, const string &name, const Point &pt) {
    string t = removeAddrTag(type);
    if(bbox_.count(t) == 0)
        bbox_[t] = unordered_map<string, BoundingBox>();
    if(bbox_[t].count(name)) {
        bbox_[t][name].update(pt);
    } else {
        bbox_[t][name] = pt;
    }
}
/**
 * @brief validAddressTag
 * @param type
 * @param name
 * @return
 */
bool AddressDecoderPlugin::validAddressTag(const string &type, const string &name) {
    // fixing german state=de bug
    if(type == "state" || type == "addr:state")
        if(!tagFilter_.isCountryCode(name))
            return false;
    return tagFilter_.isAddressTag(type) || tagFilter_.isAddressTag("addr:"+type);
}
/**
 * @brief AddressDecoderPlugin::notifyNode
 */
void AddressDecoderPlugin::notifyNode(OSMNode* node) {
    // various types
    string name, type;
    vector<pair<string, string> > areas;
    for(int i = 0; i < node->nTags; ++i) {
        string key = node->tags[i].key;
        string value = node->tags[i].value;
        if(key == "name")
            name = value;
        if(key == "place")
            type = value;
        else if(tagFilter_.isAddressArea(key)) {
            areas.push_back({key, value});
        }
    }

    Point pt = Point(node->dfLat, node->dfLon);
    if(name != "" && type != "") {
        if(validAddressTag(type, name))
            addAddressArea(type, toLower(name), pt);
    }
    for(int i = 0; i < areas.size(); i++)
        if(validAddressTag(areas[i].first, toLower(areas[i].second)))
            addAddressArea(areas[i].first, toLower(areas[i].second), pt);
}
/**
 * @brief AddressDecoderPlugin::notifyWay
 */
void AddressDecoderPlugin::notifyWay(OSMWay* way) {
    vector<VertexId> vers;
    for(int i=0; i < way->nRefs; ++i) {
        // both nodes should be present
        Vertex curr = osmValidator_->findVertex(way->nodeRefs[i]);
        if(curr.getId() == Vertex::NullVertexId)
            return;
        vers.push_back(curr.getId());
    }

    // get all points at once
    vector<Point> pts;
    vector<int8_t> found;
    vptIndex_.getPoints(vers, pts, found);

    string name, type;
    vector<pair<string, string> > areas;
    for(int i = 0; i < way->nTags; ++i) {
        if(found[i]) {
            string key = way->tags[i].key;
            string value = way->tags[i].value;
            if(key == "name")
                name = value;
            if(key == "place")
                type = value;
            else if(tagFilter_.isAddressArea(key)) {
                areas.push_back({key, value});
            }
        }
    }

    if(name != "" && type != "") {
        for(int j = 0; j < pts.size(); j++) {
            if(found[j]) {
                Point pt = pts[j];
                if(validAddressTag(type, name))
                    addAddressArea(type, toLower(name), pt);
            }
        }
    }
    for(int j = 0; j < pts.size(); j++) {
        if(found[j]) {
            Point pt = pts[j];
            for(int i = 0; i < areas.size(); i++)
                if(validAddressTag(areas[i].first, toLower(areas[i].second)))
                    addAddressArea(areas[i].first, toLower(areas[i].second), pt);
        }
    }
}
/**
 * @brief AddressDecoderPlugin::finalize
 */
void AddressDecoderPlugin::finalize() {
    // create a kdtree for each area level in the hierarhy of
    // address areas
    int currId = 0;
    map<string, int> treeToIndex;
    vector<KdTreeSpatial> kdTrees(bbox_.size());
    for(const pair<string, unordered_map<string, BoundingBox> > &atype : bbox_) {
        int currTree = treeToIndex.size();
        treeToIndex[atype.first] = currTree;

        // bulk load points with a specific type and name
        string fName = outputFileName_+".addrdecode.area."+atype.first;
        kdTrees[currTree].initForBulkLoad(fName, fName);
        for(const pair<string, BoundingBox> &name : atype.second) {
            kdTrees[currTree].insertBulk(currId++, name.second, " ");
        }
        kdTrees[currTree].closeBulk();

        // create the kd tree index
        kdTrees[currTree].bulkLoadTreeFile<BulkBoundBoxDataStream>();
        kdTrees[currTree].unload();
        currTree++;
    }

    // initialize all the trees
    for(const pair<string, unordered_map<string, BoundingBox> > &atype : bbox_) {
        int currTree = treeToIndex[atype.first];
        string fName = outputFileName_+".addrdecode.area."+atype.first;
        kdTrees[currTree].init<InputDataStream>(fName, 0);
    }

    // serialize each area with a bounding box and a parent area
    string sep = StringConsts::SEPARATOR;
    string pSpec2 = type_spec<Vertex::VertexId>()+sep+type_spec<string>()+
            sep+type_spec<int>()+sep+type_spec<string>()+
            sep+type_spec<Vertex::VertexId>()+sep+type_spec<double>()+
            sep+type_spec<double>()+sep+type_spec<double>()+
            sep+type_spec<double>()+"\n";

    currId = 0;
    for(const pair<string, unordered_map<string, BoundingBox> > &atype : bbox_) {
        // for each item find the parent area in the hierarchy, such
        // that the parent area is good
        vector<string> parAreas = tagFilter_.getParentAreas(atype.first);
        for(const pair<string, BoundingBox> &name : atype.second) {
            BoundingBox b = name.second;
            OsmGraphCore::NearestPointResult res;
            int level = tagFilter_.getAddressAreaLevel(atype.first);
            VertexPoint vpt(Vertex(-1), b.getCenter());
            Vertex::VertexId parent = currId;

            // find a parent area that is nearest to the current area
            for(const string &area : parAreas) {
                if(treeToIndex.count(area)) {
                    int currTree = treeToIndex[area];
                    if(kdTrees[currTree].findNearestVertex(vpt, res)) {
                        fprintf(addrAreasFile_, pSpec2.c_str(), currId,
                                atype.first.c_str(), level, StringUtils::quote(name.first).c_str(),
                                res.getTarget().getId(), b.getHiLeft().lat(), b.getHiLeft().lon(),
                                b.getLoRight().lat(), b.getLoRight().lon());
                        parent = res.getTarget().getId();
                        break;
                    }
                }
            }
            // no parent area was found
            if(parent == currId) {
                fprintf(addrAreasFile_, pSpec2.c_str(), currId,
                        atype.first.c_str(), level, name.first.c_str(),
                        parent, b.getHiLeft().lat(), b.getHiLeft().lon(),
                        b.getLoRight().lat(), b.getLoRight().lon());
            }
            currId++;
        }
    }

    // unload all of the kdtrees
    for(const pair<string, unordered_map<string, BoundingBox> > &atype : bbox_) {
        int currTree = treeToIndex[atype.first];
        kdTrees[currTree].unload();
    }

    vptIndex_.close();
    if(addrAreasFile_)
        fclose(addrAreasFile_);
}
/**
 * @brief AddressDecoderPlugin::cleanUp
 */
void AddressDecoderPlugin::cleanUp() {
    if(dbConn_)
        dbConn_->close();
    std::remove(areasOutFileName_.c_str());
    std::remove(translateDbName_.c_str());
    for(const pair<string, unordered_map<string, BoundingBox> > &atype : bbox_) {
        // bulk load points with a specific type and name
        string fName = outputFileName_+".addrdecode.area."+atype.first;
        std::remove((fName+".index.dat").c_str());
        std::remove((fName+".index.idx").c_str());
        std::remove(fName.c_str());
    }
}
/**
 * @brief AddressDecoderPlugin::getTableNamesToImport
 * @return
 */
std::vector<std::string> AddressDecoderPlugin::getTableNamesToImport() const {
    vector<string> tables = {SqlConsts::ADDRESS_DECODER_AREAS_TABLE};
    return tables;
}
/**
 * @brief AddressDecoderPlugin::getSqlToCreateTables
 * @return
 */
std::vector<std::string> AddressDecoderPlugin::getSqlToCreateTables() const {
    vector<string> sql = {SqlConsts::CREATE_ADDRESS_DECODER_AREA_INDEX};
    return sql;
}
/**
 * @brief AddressDecoderPlugin::getOtherSqlCommands
 * @return
 */
std::vector<std::string> AddressDecoderPlugin::getOtherSqlCommands() const {
    return {SqlConsts::UPDATE_ADDR_DECODE_AREAS_LOWER_CASE,
            SqlConsts::UPDATE_ADDR_DECODE_AREAS_TYPES_LOWER_CASE};
}

