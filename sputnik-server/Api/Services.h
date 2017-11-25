#ifndef SERVICES_H
#define SERVICES_H

#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/GraphCore/SearchResult.h>
#include <UrbanLabs/Sdk/GraphCore/Model.h>
#include <UrbanLabs/Sdk/GraphCore/ModelGTFS.h>
#include <UrbanLabs/Sdk/GraphCore/Graph.h>
#include <UrbanLabs/Sdk/Storage/Storage.h>
#include <UrbanLabs/Sdk/Storage/AddressDecoder.h>
#include <UrbanLabs/Sdk/Storage/ObjectIdStorage.h>
#include <UrbanLabs/Sdk/Utils/ObjectPool.h>
#include <UrbanLabs/Sdk/Config/ConfigManager.h>
#include <UrbanLabs/Sdk/Concurrent/LongTask.h>
#include <TileServer/TileServer.h>

class Service {
private:
    typedef Graph<AdjacencyList> Osm;
    typedef Graph<AdjacencyListGTFS> Gtfs;
    typedef TagStorage Objects;
    typedef VertexToPointIndexSql VtoPt;
    typedef OsmGraphCore::NearestPointResult NearestPointResult;
private:
    std::mutex lock_;
    std::mutex osmLock_;
    std::mutex gtfsLock_;
    std::mutex objectsLock_;
    std::mutex kdTreeSqlLock_;
    std::mutex addrDecodeLock_;
private:
    ConfigManager config_;
    FilePathCache fsCache_;
    TileServer tileServer_;
    LongTaskService longTaskService_;

    // object pools
    ObjectPool<Objects, typename Objects::Initializer, typename Objects::Destructor> objects_;
    ObjectPool<VtoPt, typename VtoPt::Initializer, typename VtoPt::Destructor> vertexToPt_;
    ObjectPool<Gtfs, typename Gtfs::Initializer, typename Gtfs::Destructor> gtfs_;
    ObjectPool<Osm, typename Osm::Initializer, typename Osm::Destructor> osm_;
    ObjectPool<ObjectIdStorage, typename ObjectIdStorage::Initializer, typename ObjectIdStorage::Destructor> objectToId_;
    ObjectPool<KdTreeSql, typename KdTreeSql::Initializer, typename KdTreeSql::Destructor> kdTreeSql_;
    ObjectPool<AddressDecoder, typename AddressDecoder::Initializer, typename AddressDecoder::Destructor> addrDecode_;
    std::string workDir_;
public:
    const static int DEFAULT_SEARCH_RESULT_LIMIT = 10;
    const static int DEFAULT_SEARCH_RESULT_OFFSET = 0;
public:
    /**
     * @brief Service
     */
    Service();
    /**
     * @brief existsGraph
     * @param name
     * @param type
     * @return
     */
    bool existsGraph(const std::string  &name, const std::string &type);
    /**
     * @brief addGraph
     * @param name
     * @param type
     * @return
     */
    bool addGraph(const std::string &name, const std::string &type);
    /**
     * @brief removeGraph
     * @param name
     * @param type
     * @return
     */
    bool removeGraph(std::string &name, const std::string &type);
    /**
     * @brief getOsmGraph
     * @param name
     * @return
     */
    Graph <AdjacencyList> &getOsmGraph(const std::string &name);
    /**
     * @brief getGtfsGraph
     * @param name
     * @return
     */
    Graph <AdjacencyListGTFS> &getGtfsGraph(const std::string &name);
    /**
     * @brief getLoadeGraphs
     * @param type
     * @return
     */
    std::vector<std::string> getLoadedGraphs(const std::string &type);
    /**
     * @brief setWorkDir
     * @param wd
     */
    void setWorkDir(const std::string &wd);
public:
    /**
     * @brief resolveToOsmIds
     * @param url
     * @param tags
     */
    bool resolveToOsmIds(const URL &url, std::vector<TagList> &tags);
    /**
     * @brief resolveFromOsmIds
     * @param mapname
     * @param from
     * @param to
     */
    bool resolveFromOsmIds(const std::string &mapname, const std::vector<string> &from, std::vector<Vertex::VertexId> &to);
    /**
     * @brief simpleSearch
     * @param mapName
     * @param table
     * @param cond
     * @param limit
     * @param offset
     * @param resolveLatLon
     * @param tags
     * @return
     */
    bool simpleSearch(const std::string &mapName, const std::string &table,
                      ConditionContainer &cond, int offset, int limit, std::vector<TagList> &tags);

    /**
     * @brief simpleSearch
     * @param mapName
     * @param table
     * @param cond
     * @param limit
     * @param offset
     * @param resolveLatLon
     * @param tags
     * @return
     */
    bool fulltextSearch(const std::string &mapName, const std::string &table,
                        ConditionContainerFullText &cond, int offset, int limit,
                        std::vector<TagList> &tags);
    /**
     * @brief writeData
     * @param mapName
     * @param table
     * @param tagList
     * @return
     */
    bool writeData(const std::string &mapName, const std::string &table, const TagList &tagList);
    /**
     * @brief findNearestPoint
     * @param mapName
     * @param mapType
     * @param pt
     * @param result
     * @return
     */
    bool findNearestPoint(const std::string &mapName, const std::string &mapType,
                          const Point &pt, NearestPointResult &result);
    /*
     * @brief findNearestObject
     * @param mapName
     * @param pt
     * @param result
     * @return
     */
    bool findNearestObject(const std::string &mapName,
                           const Point &pt, NearestPointResult &result);
    /**
     * @brief findObjectsInBoundingBox
     * @param mapName
     * @param hiLeft
     * @param lowRight
     * @param vpts
     * @return
     */
    bool findObjectsInBoundingBox(const std::string &mapName,
                                  const Point &hiLeft, const Point lowRight,
                                  std::vector<VertexPoint> &vpts);
    /**
     * @brief findAddressDecodable
     * @param mapName
     * @param pt
     * @param result
     * @return
     */
    bool decodeAddress(const std::string &mapName, const std::vector<Point> &pt, std::vector<TagList> &result);
    /**
     * @brief findCoordinates
     * @param mapName
     * @param ids
     * @param pts
     * @return
     */
    bool findCoordinates(const std::string &mapName, const std::vector<Vertex::VertexId> &ids,
                         std::vector<Point> &pts, std::vector<int8_t> &found);

    /**
     * @brief getTile
     * @param x
     * @param y
     * @param z
     * @param ext
     * @return
     */
    std::string getTileString(const std::string& mapFile, const std::string &layerId, int x, int y, int z, bool force, const string &ext = "png");
    mapnik::image_32 getTileBitmap(const std::string& mapFile, const std::string &layerId, int x, int y, int z);

    /**
     * @brief loadTiles
     * @param mapName
     * @return
     */
    bool loadTiles(const std::string &mapName);
    /**
     * @brief setZoom
     * @param mapName
     * @param zoom
     */
    bool setZoom(const std::string &mapName, int zoom);
    /**
     * @brief findFile
     * @param fileName
     * @param fullPathToFile
     * @return
     */
    bool findFile(const std::string& fileName, std::string &fullPathToFile);
    /**
     * @brief getAvailableMaps
     * @param maps
     * @return
     */
    bool getOfflineMaps(const std::string &searchTerm, const std::set<std::string>& searchDirs, std::vector<std::string> &maps);
    /**
     * @brief matchTags
     * @param mapName
     * @param tag
     * @param tags
     * @return
     */
    bool matchTags(const std::string &mapName, const std::string &tag, std::vector<std::string> &tags);

    /**
     * @brief getMapIcon
     * @param mapName
     * @param img
     * @return
     */
    bool getMapIcon(const std::string &mapName, string &img);
    /**
     * @brief downloadFile
     * @param mapName
     * @param taskId
     * @return
     */
    bool downloadFile(const std::string &mapName, int &taskId);
    /**
     * @brief checkAvailableDiskSpace
     * @param path
     * @param megabytesRequired
     * @return
     */
    bool checkAvailableDiskSpace(const std::string &path, size_t megabytesRequired);
    /**
     * @brief decompressFile
     * @param fileName
     * @param taskId
     * @return
     */
    bool decompressFile(const std::string &fileName, int &taskId);
    /**
     * @brief getTaskStatus
     * @param taskId
     * @param percent
     * @return
     */
    bool getTaskStatus(int taskId, double &percent, std::string &status, std::string &error);
    /**
     * @brief getConfig
     * @param tags
     * @return
     */
    bool getConfig(std::vector<TagList> &tags);
};

#endif // SERVICES_H
