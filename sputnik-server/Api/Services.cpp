#include <Api/Services.h>
#include <UrbanLabs/Sdk/Storage/TagSuggestStorage.h>
#include <UrbanLabs/Sdk/Storage/AddressDecoder.h>
#include <UrbanLabs/Sdk/Storage/ConnectionsManager.h>

using namespace std;

/**
 * @brief Service::Service
 */
Service::Service() : lock_(), config_(), tileServer_(), workDir_()  {
    ;
}
/**
 * Resolve string id to osm id
 */
bool resolveOsmIdType(const string &from, Vertex::VertexId &id, string &type) {
    if(from.size() == 0)
        return false;
    type = from[0];
    if(type == "n")
        type = "node";
    else if(type == "w")
        type = "way";
    else return false;
    id = lexical_cast<Vertex::VertexId>(from.substr(1));
    return true;
}
/**
 * @brief Service::setWorkDir
 * @param wd
 */
void Service::setWorkDir(const string &workDir) {
    workDir_ = workDir;

    // locate the configuration file
    string configPath;
    if(!findFile("sputnik.config", configPath)) {
        LOGG(Logger::ERROR) << "couldn't find config file" << Logger::FLUSH;
    }

    // set the configuration
    config_.close();
    URL configURL(configPath);
    Properties props = {{"type", "sqlite"}, {"memory", "0"}, {"create", "0"}, {"readwrite", "1"}};
    if(!config_.open(configURL, props)) {
        LOGG(Logger::ERROR) << "Couldn't open config" << Logger::FLUSH;
    }
    tileServer_.setWorkDir(workDir);
}
/**
 * @brief Service::existsGraph
 * @param name
 * @param type
 * @return
 */
bool Service::existsGraph(const string &name, const string &type) {
    bool ret = false;
    if(type == "osm") {
        ret = osm_.existsObj(name);
    }
    else if(type == "gtfs") {
        ret = gtfs_.existsObj(name);
    }
    return ret;
}
/**
 * @brief Service::addGraph
 * @param name
 * @param type
 * @return
 */
bool Service::addGraph(const string &name, const string &type) {
    Timer timer;
    string realPath;
    if (!findFile(name, realPath)) {
        return false;
    }
    bool ret = false;
    if(type == "osm") {
        Graph<AdjacencyList>::Initializer init(realPath);
        ret = osm_.addObj(name, init);
    }
    else if(type == "gtfs") {
        Graph<AdjacencyListGTFS>::Initializer init(realPath);
        ret = gtfs_.addObj(name, init);
    }
    timer.stop();
    LOGG(Logger::INFO) << "[ADD GRAPH]: " << timer.getElapsedTimeSec() << Logger::FLUSH;
    return ret;
}
/**
 * @brief Service::removeGraph
 * @param name
 * @param type
 * @return
 */
bool Service::removeGraph(string &name, const string &type) {
    if(type == "osm")
        return osm_.removeObj(name);
    else if(type == "gtfs")
        return gtfs_.removeObj(name);
    return false;
}
/**
 * @brief Service::getOsmGraph
 * @param name
 * @return
 */
Graph <AdjacencyList> &Service::getOsmGraph(const string &name) {
    return osm_.getObj(name);
}
/**
 * @brief Service::getGtfsGraph
 * @param name
 * @return
 */
Graph <AdjacencyListGTFS> &Service::getGtfsGraph(const string &name) {
    return gtfs_.getObj(name);
}
/**
 * @brief getLoadedOsm
 * @return
 */
vector<string> Service::getLoadedGraphs(const string &type) {
    if(type == "osm")
        return osm_.getLoadedObj();
    else if(type == "gtfs")
        return gtfs_.getLoadedObj();
    return {};
}
/**
 * @brief Service::findNearestPoint
 * @param mapName
 * @param pt
 * @param result
 * @return
 */
bool Service::findNearestPoint(const string &mapName,const string &mapType,
                               const Point &pt, NearestPointResult &result) {
    Timer timer;
    if(mapType == "osm") {
        lock_guard<mutex> lock(osmLock_);
        if(!osm_.getObj(mapName).findNearestPoint(pt, result))
            return false;
    }
    if(mapType == "gtfs") {
        lock_guard<mutex> lock(gtfsLock_);
        if(!gtfs_.getObj(mapName).findNearestPoint(pt, result))
            return false;
    }

    timer.stop();
    LOGG(Logger::INFO) << "[FIND NEAREST POINT]: " << timer.getElapsedTimeSec() << Logger::FLUSH;

    return true;
}
/**
 * @brief Service::findNearestObject
 * @param mapName
 * @param pt
 * @param result
 * @return
 */
bool Service::findNearestObject(const string &mapName, const Point &pt, NearestPointResult &result) {
    Timer timer;

    string realPath;
    if(!findFile(mapName, realPath)) {
        return false;
    }
    // deserialize vertices
    URL url(realPath);
    // {{"type", "sqlite"}, {"create", "0"}, {"table", SqlConsts::KDTREE_OBJECTS_INDEX_TABLE}};
    Properties objProps;
    string service = "findNearestObject";
    if(!config_.get(service, objProps))
        return false;

    string kdTreeName = mapName+service;
    KdTreeSql::Initializer init(url, objProps, false);

    lock_guard<mutex> lock(kdTreeSqlLock_);
    if(kdTreeSql_.existsObj(kdTreeName) || kdTreeSql_.addObj(kdTreeName, init)) {
        string data;
        if(!kdTreeSql_.getObj(kdTreeName).findNearestVertex(pt, result, data))
            return false;
    }

    timer.stop();
    LOGG(Logger::INFO) << "[FIND NEAREST OBJECT]: " << timer.getElapsedTimeSec() << Logger::FLUSH;
    return true;
}
/**
 * @brief resolveToOsmIds
 * @param url
 * @param tags
 */
bool Service::resolveToOsmIds(const URL &url, vector<TagList> &tags) {
    // resolve internal ids
    Properties props = {{"create","0"},{"type","sqlite"},{"memory", "0"},{"objectidstorage.table","osm_id"},
                        {"objectidstorage.innerid","internal_id"},{"objectidstorage.outerid","osm_id"}};
    ObjectIdStorage storage;
    if(!storage.open(url, props)) {
        LOGG(Logger::ERROR) << "Couldn't open object id storage" << Logger::FLUSH;
        return false;
    }

    vector<string> to;
    vector<Vertex::VertexId> resolve;
    for(int i = 0; i < tags.size(); i++) {
        resolve.push_back(tags[i].getId());
        vector<Vertex::VertexId> obj = tags[i].getObjects();
        resolve.insert(resolve.end(), obj.begin(), obj.end());
    }

    bool res = true;
    if(!storage.fromInternal(resolve, to)) {
        LOGG(Logger::ERROR) << "Couldn't resolve object id storage" << Logger::FLUSH;
        res = false;
    }

    for(int i = 0, j = 0; i < tags.size() && res; i++) {
        string type;
        Vertex::VertexId outer;
        if(!resolveOsmIdType(to[j++], outer, type)) {
            LOGG(Logger::ERROR) << "Failed to resolve osm id " << to[j] << Logger::FLUSH;
            outer = Vertex::NullVertexId;
        }
        
        tags[i].setId(outer);
        tags[i].add({"osmtype", type});
        vector<Vertex::VertexId> members = tags[i].getObjects();
        for(int k = 0; k < members.size(); k++, j++) {
             if(!resolveOsmIdType(to[j], outer, type)) {
                LOGG(Logger::ERROR) << "Failed to resolve osm id " << to[j] << Logger::FLUSH;
                outer = Vertex::NullVertexId;
            }
            members[k] = outer;
        }
        tags[i].setObjects(members);
    } 
    return res;
}
/**
 * @brief resolveOsmIds
 * @param url
 * @param tags
 */
bool Service::resolveFromOsmIds(const string &mapName, const vector<string> &from, vector<Vertex::VertexId> &to) {
    string realPath;
    if(!findFile(mapName, realPath)) {
        return false;
    }

    URL url(realPath);
    // resolve internal ids
    Properties props = {{"create","0"},{"type","sqlite"},{"memory", "0"},{"objectidstorage.table","osm_id"},
                        {"objectidstorage.innerid","internal_id"},{"objectidstorage.outerid","osm_id"}};
    ObjectIdStorage storage;
    if(!storage.open(url, props)) {
        LOGG(Logger::ERROR) << "Couldn't open object id storage" << Logger::FLUSH;
        return false;
    }

    if(!storage.toInternal(from, to)) {
        LOGG(Logger::ERROR) << "Couldn't resolve object id storage" << Logger::FLUSH;
        return false;
    }

    return true;
}
/**
 * @brief Service::findCoordinates
 * @param mapName
 * @param table
 * @param resolveLatLon
 * @param ids
 * @param pts
 * @return
 */
bool Service::findCoordinates(const string &mapName, const vector<Vertex::VertexId> &ids,
                              vector<Point> &pts, vector<int8_t> &found) {
    Timer timer;
    found = vector<int8_t>(ids.size(), 0);

    string realPath;
    if(!findFile(mapName, realPath)) {
        return false;
    }

    URL url(realPath);
    // {{"type", "sqlite"}, {"table", SqlConsts::KDTREE_OBJECTS_INDEX_TABLE},
    // {"idcol", SqlConsts::KDTREE_OBJECTS_INDEX_TABLE_ID_COL}, {"create", "0"}};
    Properties props;
    string service = "findCoordinates";
    if(!config_.get(service, props))
        return false;

    string kdTreeName = mapName+service;
    KdTreeSql::Initializer init(url, props, false);

    lock_guard<mutex> lock(kdTreeSqlLock_);
    if(kdTreeSql_.existsObj(kdTreeName) || kdTreeSql_.addObj(kdTreeName, init)) {
        if(!kdTreeSql_.getObj(kdTreeName).getPoints(ids, pts, found))
            return false;
    }

    timer.stop();
    LOGG(Logger::INFO) << "[FIND COORDINATES]: " << timer.getElapsedTimeSec() << Logger::FLUSH;

    return true;
}
/**
 * @brief Service::simpleSearch
 * @param mapName
 * @param table
 * @param ids
 * @param conds
 * @param limit
 * @param offset
 * @param resolveLatLon
 * @param tags
 * @return
 */
bool Service::simpleSearch(const string &mapName, const string &table, ConditionContainer &conds,
                           int offset, int limit, vector<TagList> &tags) {
    Timer timer;

    // preliminary verification
    set<string> validTables = {SqlConsts::GTFS_TAGS_TABLE, SqlConsts::OSM_TAG_TABLE, SqlConsts::MAP_INFO_TABLE};
    if(validTables.count(table) == 0) {
        LOGG(Logger::ERROR) << "Wrong table name" << Logger::FLUSH;
        return false;
    }

    // find the full path to storage file
    string realPath;
    if(!findFile(mapName, realPath)) {
        return false;
    }

    URL tagSearchUrl(realPath);
    // {{"type", "sqlite"}, {"joinspatial", "0"}}
    Properties tagSearchProps;
    if(!config_.get("simpleSearch", tagSearchProps))
        return false;

    TagStorage::Initializer init(tagSearchUrl, tagSearchProps);

    lock_guard<mutex> lock(objectsLock_);
    if(objects_.existsObj(mapName) || objects_.addObj(mapName, init)) {
        // extract tag lists
        if(!objects_.getObj(mapName).simpleSearch(table,conds,offset,limit,tags)) {
            LOGG(Logger::ERROR) << "Couldn't find objects with tags" << Logger::FLUSH;
            return false;
        }
    }

    timer.stop();
    LOGG(Logger::INFO) << "[FIND OBJECTS WITH TAGS]: " << timer.getElapsedTimeSec() << Logger::FLUSH;
    return true;
}
/**
 * @brief Service::simpleSearch
 * @param mapName
 * @param table
 * @param conds
 * @param limit
 * @param offset
 * @param resolveLatLon
 * @param tags
 * @return
 */
bool Service::fulltextSearch(const string &mapName, const string &table, ConditionContainerFullText &cont,
                             int offset, int limit, vector<TagList> &tags) {

    Timer timer;
    // preliminary verification
    set<string> validTables = {SqlConsts::OSM_TAG_TABLE};
    if(validTables.count(table) == 0) {
        LOGG(Logger::ERROR) << "Wrong table name" << Logger::FLUSH;
        return false;
    }

    // find the full path to storage file
    string realPath;
    if(!findFile(mapName, realPath)) {
        return false;
    }

    URL tagSearchUrl(realPath);
    // {{"type", "sqlite"}, {"joinspatial", "0"}}
    Properties tagSearchProps;
    if(!config_.get("simpleSearch", tagSearchProps))
        return false;

    TagStorage::Initializer init(tagSearchUrl, tagSearchProps);

    lock_guard<mutex> lock(objectsLock_);
    if(objects_.existsObj(mapName) || objects_.addObj(mapName, init)) {
        if(!objects_.getObj(mapName).fulltextSearch(table,cont,offset,limit,tags)) {
            LOGG(Logger::ERROR) << "Couldn't find objects with tags" << Logger::FLUSH;
            return false;
        } else {
            // try to set coordinates to tag list
            vector<Vertex::VertexId> tagIds;
            for(int i = 0; i < tags.size(); i++) {
                tagIds.push_back(tags[i].getId());
            }
            vector<Point> pts;
            vector<int8_t> found;
            if(!findCoordinates(mapName, tagIds, pts, found)) {
                LOGG(Logger::WARNING) << "Not all points were found" << Logger::FLUSH;
            }
            for(int i = 0; i < tags.size(); i++) {
                if(found[i])
                    tags[i].setPoint(pts[i]);
            }
        }
    }

    timer.stop();
    LOGG(Logger::INFO) << "[FIND OBJECTS WITH TAGS]: " << timer.getElapsedTimeSec() << Logger::FLUSH;
    return true;
}
/**
 * @brief Service::findObjectsInBoundingBox
 * @param mapName
 * @param hiLeft
 * @param lowRight
 * @param vpts
 * @return
 */
bool Service::findObjectsInBoundingBox(const string &mapName,
                                       const Point &hiLeft, const Point lowRight,
                                       vector<VertexPoint> &vpts) {
    Timer timer;

    // find the full path to storage file
    string realPath;
    if(!findFile(mapName, realPath)) {
        return false;
    }

    URL url(realPath);
    // {{"type", "sqlite"}, {"create", "0"}, {"table", SqlConsts::KDTREE_OBJECTS_INDEX_TABLE}}
    Properties objProps;
    string service = "findObjectsInBoundingBox";
    if(!config_.get(service, objProps))
        return false;

    string kdTreeName = mapName+service;
    KdTreeSql::Initializer init(url, objProps, true);

    lock_guard<mutex> lock(kdTreeSqlLock_);
    if(kdTreeSql_.existsObj(kdTreeName) || kdTreeSql_.addObj(kdTreeName, init)) {
        vector<string> ptData;
        if(!kdTreeSql_.getObj(kdTreeName).findAllInBoundingBox(hiLeft, lowRight, vpts, ptData)) {
            return false;
        }
    }

    timer.stop();
    LOGG(Logger::INFO) << "[FIND OBJECTS IN BBOX]: " << timer.getElapsedTimeSec() << Logger::FLUSH;
    return true;
}
/**
 * @brief Service::decodeAddress
 * @param mapName
 * @param pt
 * @param result
 * @return
 */
bool Service::decodeAddress(const string &mapName, const vector<Point> &pt, vector<TagList> &result) {
    Timer timer;
    // find the full path to storage file
    string realPath;
    if(!findFile(mapName, realPath)) {
        return false;
    }
    URL url(realPath);

    // initialize address decoder tree
    // {{"type", "sqlite"}, {"create", "0"}, {"table", SqlConsts::KDTREE_ADDRESS_DECODER_TABLE}};
    Properties addrProps;
    string service = "findAddressDecodable";
    if(!config_.get(service, addrProps))
        return false;

    result.resize(pt.size());
    string addrDecodeName = mapName+"."+service;
    for(int i = 0; i < pt.size(); i++) {
        AddressDecoder::Initializer init(url, addrProps);
        lock_guard<mutex> lock(addrDecodeLock_);
        if(addrDecode_.existsObj(addrDecodeName) || addrDecode_.addObj(addrDecodeName, init)) {
            string data;
            if(!addrDecode_.getObj(addrDecodeName).resolve(pt[i], result[i])) {
                LOGG(Logger::INFO) << "[ADDRESS DECODE]: failed to resolve point" << Logger::FLUSH;
                return false;
            }
        }
    }

    timer.stop();
    LOGG(Logger::INFO) << "[FIND ADDRESS DECODABLE]: " << timer.getElapsedTimeSec() << Logger::FLUSH;
    return true;
}
/**
 * @brief Service::getTile
 * @param x
 * @param y
 * @param z
 * @param ext
 * @return
 */
string Service::getTileString(const string &mapFile, const string &layerId, int x, int y, int z, bool force, const string &ext) {
    return tileServer_.getTileString(mapFile, layerId, x, y, z, force, ext);
}

mapnik::image_32 Service::getTileBitmap(const std::string& mapFile, const std::string &layerId, int x, int y, int z) {
    return tileServer_.getTileBitmap(mapFile, layerId, x, y, z);
}
/**
 * @brief Service::loadTiles
 * @param mapName
 * @return
 */
bool Service::loadTiles(const string &mapName) {
    return tileServer_.loadMap(mapName);
}
/**
 * @brief Service::setZoom
 * @param mapName
 * @param zoom
 */
bool Service::setZoom(const string &mapName, int zoom) {
    return tileServer_.setZoom(mapName, zoom);
}
/**
 * @brief Service::findFile
 * @param fileName
 * @param fullPathToFile
 * @return
 */
bool Service::findFile(const string& fileName, string& fullPathToFile) {
    if(fsCache_.get(fileName, fullPathToFile))
        return true;

    if(!FsUtil::search(fileName, {workDir_}, fullPathToFile)) {
        LOGG(Logger::ERROR) << "Couldnt find data file: "<< fileName << Logger::FLUSH;
        return false;
    }
    fsCache_.put(fileName, fullPathToFile);
    return true;
}
/**
 * @brief Service::getAvailableMaps
 * @param maps
 * @return
 */
bool Service::getOfflineMaps(const string &/*searchTerm*/, const set<string>& searchDirs,
                             vector<string> &maps) {
    LOGG(Logger::DEBUG) << "Reading avaliable maps" << Logger::FLUSH;
    set<string> files;
    if(!FsUtil::listDirs(searchDirs, files)) {
        LOGG(Logger::ERROR) << "Couldn't list given directories" << Logger::FLUSH;
        return false;
    }
    Properties tagSearchProps;
    if(!config_.get("simpleSearch", tagSearchProps)) {
        LOGG(Logger::ERROR) << "Failed to get config props for simpleSearch" << Logger::FLUSH;
        return false;
    }

    for(const string& filePath : files) {
        TagStorage storage;
        URL tagSearchUrl(filePath);
        if(!storage.open(tagSearchUrl, tagSearchProps)) {
            LOGG(Logger::DEBUG) << "Couldn't open db" << filePath << Logger::FLUSH;
            continue;
        }

        ConditionContainer conds;
        conds.addIdsIn({0});

        vector<TagList> res;
        int limit = numeric_limits<int>::max(), offet = 0;
        if(storage.simpleSearch(SqlConsts::MAP_INFO_TABLE,conds,offet,limit,res)) {
            if(res.size() > 0) {
                string fileName;
                if(FsUtil::extractFileName(filePath, fileName))
                    maps.push_back(fileName);
            }
        }
    }
    return true;
}

/**
 * @brief Service::getMapIcon
 * @param mapName
 * @param img
 * @return
 */
bool Service::getMapIcon(const string &mapName, string &img) {
    // find the full path to storage file
    string realPath;
    if(!findFile(mapName, realPath)) {
        return false;
    }
    URL url(realPath);
    Properties tagSearchProps;
    if(!config_.get("simpleSearch", tagSearchProps)) {
        LOGG(Logger::ERROR) << "Failed to get config props for simpleSearch" << Logger::FLUSH;
        return false;
    }

    TagStorage::Initializer init(url, tagSearchProps);

    lock_guard<mutex> lock(objectsLock_);
    if(objects_.existsObj(mapName) || objects_.addObj(mapName, init)) {
        ConditionContainer conds;
        conds.addIdsIn({0});

        vector<TagList> res;
        int limit = numeric_limits<int>::max(), offset = 0;
        string table = SqlConsts::MAP_INFO_TABLE;
        if(!objects_.getObj(mapName).simpleSearch(table, conds, offset, limit, res)) {
            LOGG(Logger::ERROR) << "Couldn't find objects with tags for mapicon" << Logger::FLUSH;
            return false;
        } else {
            if(res.size() > 0) {
                for(const KeyValuePair &kv:res[0].getTags()) {
                    if(kv.getKey() == "bbox") {
                        SimpleTokenator tokenator(kv.getValue(), ' ', '\"', true);
                        if(tokenator.countTokens() < 4) {
                            LOGG(Logger::ERROR) << "Wrong bounding box" << kv.getValue()<< Logger::FLUSH;
                            return false;
                        }
                        vector<string> tokens = tokenator.getTokens();
                        double box[4];
                        for(int i = 0; i < 4; i++) {
                            box[i] = lexical_cast<double>(tokens[i]);
                        }
                        std::vector<Point> bbox = {Point(box[0], box[1]), Point(box[2], box[3])};
                        return tileServer_.renderIcon(realPath, bbox, img);
                    }
                } 
            }
        }
    }

    return false;
}
/**
 * @brief matchTags
 * @param mapName
 * @param tag
 * @param tags
 * @return
 */
bool Service::matchTags(const string &mapName, const string &tag, vector<string> &tags) {
    Properties match;
    if(!config_.get("matchTags", match)) {
        LOGG(Logger::ERROR) << "Failed to get config props for matchTags" << Logger::FLUSH;
        return false;
    }

    // find the full path to storage file
    string realPath;
    if(!findFile(mapName, realPath)) {
        return false;
    }

    URL url(realPath);
    TagSuggestStorage tagStore;
    if(!tagStore.open(url, match)) {
        LOGG(Logger::ERROR) << "Couldn't match tag" << Logger::FLUSH;
        return false;
    }
    if(!tagStore.matchTag(tag, tags)) {
        LOGG(Logger::ERROR) << "Couldn't match tag" << Logger::FLUSH;
        return false;
    }
    return true;
}
/**
 * @brief Service::checkAvailableDiskSpace
 * @param path
 * @param megabytesRequired
 * @return
 */
bool Service::checkAvailableDiskSpace(const string &path, size_t megabytesRequired) {
    int64_t dSize = 0, tFree = 0;
    if(FsUtil::getDiskSpace(path, dSize, tFree)) {
        LOGG(Logger::DEBUG)<<"Avalibale disk "<<dSize <<", free "<< tFree << Logger::FLUSH;
        size_t mbAvail = tFree/(1024*1024);
        if(megabytesRequired > mbAvail) {
            LOGG(Logger::DEBUG)<<"Not enough free space at "<<path<< tFree << ", need "<<megabytesRequired
                    <<", avail" << mbAvail << Logger::FLUSH;
            return false;
        }
        return true;
    }
    LOGG(Logger::ERROR)<<"Failed to get disk space "<< path << Logger::FLUSH;
    return false;
}

/**
 * @brief Service::downloadFile
 * @param mapName
 * @return
 */
bool Service::downloadFile(const string &/*mapName*/, int &/*taskId*/) {
    // Properties dProps;
    // if(!config_.get("downloadService", dProps)) {
    //     LOGG(Logger::ERROR) << "Failed to get config props for downloadService" << Logger::FLUSH;
    //     return false;
    // }
    // string url = dProps.get("downloadUrl");
    // if(url == "") {
    //     LOGG(Logger::ERROR) << "Failed to get url for downloadService" << Logger::FLUSH;
    //     return false;
    // }
    // if(url[url.size()-1] != '/')
    //     url += "/";

    // LOGG(Logger::DEBUG) << "Initializing download from "<< url << mapName << Logger::FLUSH;
    // URL dUrl(url+mapName);
    // vector<string> outPath = {workDir_, mapName};
    // URL outFile(FsUtil::makePath(outPath));
    // taskId = longTaskService_.submit(DownloadTask::run, dUrl, outFile);
    return true;
}
/**
 * @brief Service::decompressFile
 * @param fileName
 * @param taskId
 * @return
 */
bool Service::decompressFile(const string &/*fileName*/, int &/*taskId*/) {
    // find the full path to storage file
    // string realPath;
    // if(!findFile(fileName, realPath)) {
    //     return false;
    // }
    // // check available disk space
    // int64_t fSize = 0;
    // if(!FsUtil::getFileSize(realPath, fSize)) {
    //     LOGG(Logger::ERROR) << "Failed to get file size for "<< realPath << Logger::FLUSH;
    //     return false;
    // }
    // fSize = fSize/(1024*1024);
    // // decompression requires ~3 * file size
    // if(!checkAvailableDiskSpace(workDir_, 3*fSize)) {
    //     return false;
    // }
    // URL dUrl(realPath);
    // taskId = longTaskService_.submit(DecompressTask::run,dUrl,dUrl);
    return true;
}
/**
 * @brief Service::getTaskStatus
 * @param taskId
 * @param percent
 * @return
 */
bool Service::getTaskStatus(int /*taskId*/, double &/*percent*/, string &/*status*/, string &/*error*/) {
    // TaskStatus::State state(TaskStatus::UNKNOWN);
    // longTaskService_.getState(taskId,percent,state,error);
    // switch (state) {
    // case TaskStatus::FAILED:
    //     status = "failed";
    //     break;
    // case TaskStatus::UNKNOWN:
    //     status = "unknown";
    //     break;
    // case TaskStatus::INITIALIZING:
    //     status = "init";
    //     break;
    // case TaskStatus::RUNNING:
    //     status = "running";
    //     break;
    // case TaskStatus::SUCCESS:
    //     status = "success";
    //     break;
    // default:
    //     break;
    // }
    return true;
}
/**
 * @brief Service::writeData
 * @param table
 * @param tagList
 * @return
 */
bool Service::writeData(const string &file, const string &table, const TagList &tagList) {
    // preliminary verification
    set<string> validTables = {SqlConsts::CONFIG_TABLE};
    if(validTables.count(table) == 0) {
        LOGG(Logger::ERROR) << "Wrong table name" << Logger::FLUSH;
        return false;
    }

    // if this is the config table, do clear the configuration cache
    if(table == SqlConsts::CONFIG_TABLE) {
        config_.clear("writeData");
        objects_.removeObj(SqlConsts::CONFIG_TABLE);
    }

    // find the full path to storage file
    string realPath;
    if(!findFile(file, realPath)) {
        return false;
    }

    URL url(realPath);
    // {{"type", "sqlite"}, {"create", "0"}}
    Properties writeDataProps;
    if(!config_.get("writeData", writeDataProps))
        return false;

    lock_guard<mutex> lock(objectsLock_);
    TagStorage::Initializer init(url, writeDataProps);
    if(objects_.existsObj(file) || objects_.addObj(file, init)) {
        if(!objects_.getObj(file).write(table, tagList)) {
            LOGG(Logger::ERROR) << "Couldn't write data" << Logger::FLUSH;
            return false;
        }
    }
    return true;
}
/**
 * @brief Service::getConfig
 * @param tags
 * @return
 */
bool Service::getConfig(vector<TagList> &tags) {
    Properties dProps;
    if(!config_.get("downloadService", dProps)) {
        LOGG(Logger::ERROR) << "Failed to get config props for downloadService" << Logger::FLUSH;
        return false;
    }
    map<string,string> props = dProps.list();
    map<string,string>::const_iterator it = props.begin();
    TagList tg;
    for(; it != props.end();++it) {
        KeyValuePair kv((*it).first, (*it).second);
        tg.add(kv);
    }
    tags.push_back(tg);
    return true;
}
