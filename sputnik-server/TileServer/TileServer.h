#pragma once

#include <array>
#include <queue>
#include <mutex>
#include <chrono>
#include <unordered_map>

#include <boost/optional.hpp>

#include <google/dense_hash_map>
#include <google/dense_hash_set>

#include <mapnik/version.hpp>
#include <mapnik/map.hpp>
#if MAPNIK_VERSION >= 300000
    #define image_data_32 image_rgba8
    #define image_32 image_rgba8
    #include <mapnik/image.hpp>
#else
    #include <mapnik/graphics.hpp>
    #include <mapnik/box2d.hpp>
#endif

#include <UrbanLabs/Sdk/GraphCore/Point.h>
#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/Utils/ThreadSafe.h>
#include <UrbanLabs/Sdk/Utils/Properties.h>
#include <UrbanLabs/Sdk/Concurrent/ReadWriteLock.h>

/**
 * @brief The Pixel class
 */
struct Pixel {
    int x, y;
};

/**
 * @brief The TilePos class
 */
struct TilePos {
    int x, y, z;
};

/**
 * @brief The TileCacheEntry class
 */
class TileCacheEntry {
private:
    std::string name_;
    std::chrono::system_clock::time_point stamp_;

public:
    bool operator < (const TileCacheEntry& entry) const;
    TileCacheEntry(const std::string& name);
    std::string getName() const;
};

namespace std {
// specialize hashing function for TileCacheEntry
template <>
class hash<TileCacheEntry> {
public:
    size_t operator()(const TileCacheEntry& entry) const
    {
        return hash<string>()(entry.getName());
    }
};
}
/**
 * @brief The TileMemoryCache class
 */
class TileMemoryCache : ReadWriteLock {
public:
    typedef TileCacheEntry Entry;

private:
    int maxSize_;
    std::string workDir_;
    std::priority_queue<Entry, std::vector<Entry> > cache_;
    google::dense_hash_map<std::string, std::string> ids_;

public:
    TileMemoryCache(int maxSize);
    void putWithContent(const std::string& name, const std::string& content);
    bool get(const std::string& name, std::string& content);
};
/**
 * @brief The TileCache class
 */
class TileExternalCache : ReadWriteLock {
public:
    typedef TileCacheEntry Entry;

protected:
    int maxSize_;
    std::string workDir_;
    std::priority_queue<Entry, std::vector<Entry> > cache_;
    google::dense_hash_set<std::string> ids_;

public:
    TileExternalCache(int maxSize);
    void put(const std::string& name);
    void putWithContent(const std::string& name, const std::string& content);
    bool get(const std::string& name, std::string& content);
};
/**
 * Latlon <-> pixel conversion
 * @brief The GoogleProjection class
 */
class GoogleProjection {
public:
    constexpr static double RAD_TO_DEG = 180.0 / M_PI;
    constexpr static double DEG_TO_RAD = M_PI / 180.0;

public:
    std::vector<double> pixPerDegAtZoom, pixPerRadAtZoom, pixAtZoom;
    std::vector<std::pair<int, int> > pixDiv2AtZoom;

public:
    GoogleProjection(int levels);
    double miniMax(double x, double y, double z) const;
    Pixel fromLatLonToPixel(const Point& latLon, int zoom) const;
    Point fromPixelToLatLon(const Pixel& px, int zoom) const;
};
/**
 * @brief The TileCache class
 */
class TileCache {
private:
    std::string workDir_;
    TileExternalCache metaCache_;
    TileMemoryCache metaMemCache_;
private:
    bool checkInMemory(const std::string& fullPath, std::string& content);
    bool checkOnDisk(const std::string& fullPath, std::string& content);
public:
    TileCache(const std::string& workDir);
    void put(const std::string& fileName, const std::string& tile);
    void put(const std::string& filename);
    boost::optional<std::string> get(const std::string& fileName);
};
/**
 * @brief The Layer class
 */
class Layer : ReadWriteLock {
public:
    const static int TILE_SIZE = 256;

private:
    std::string id_;
    TileCache cache_;
    std::string workDir_;
    static const GoogleProjection epsg3875_;
    std::vector<mapnik::Map> metaMaps_;
    std::vector<std::mutex> mapLocks_;

public:
    int NUM_MAPS;
    int NUM_META;
    int METATILE_SIZE;
    int METATILE_MARGIN;

private:
    mapnik::box2d<double> getBbox(int x, int y, int z) const;
    mapnik::box2d<double> getBboxIgnoreMeta(int x, int y, int z) const;
    int getMetaId(int x, int y, int z) const;

public:
    // layer state initialization
    // these methods are thread unsafe and must be protected by a mutex
    Layer(const std::string& id, const std::string& workDir, int numMeta, int margin, int numMaps = 2);
    void load(const std::string& xml);
    void readExisting();

    // these methods are thread safe
    std::string renderTile(int x, int y, int z, const std::string& ext);
    mapnik::image_32 renderTile(int x, int y, int z);
    static TilePos tilePosForPoint(const Point &pt, int zoom);
    bool isValidTileName(const std::string& fileName) const;
    std::string extractLayerId(const std::string& fileName) const;
    // current layer state
    void setZoom(int zoom);
    std::string getId() const;
    int getZoom() const;
};
/**
 * @brief The MapContainer class
 */
class MapContainer : ReadWriteLock {
private:
    int zoom_;
    std::string name_;
    std::map<std::string, Layer> layers_;

public:
    const static int DEFAULT_ZOOM = 14;

public:
    // thread unsafe
    MapContainer(const std::string& name);
    void addLayer(Layer&& layer);

    // thread safe
    std::string getTileString(const std::string& id, int x, int y, int z, bool force, const std::string& ext);
    mapnik::image_32 getTileBitmap(const std::string& id, int x, int y, int z);
    std::string getName() const;
    void setZoom(int zoom);
};
/**
 * @brief The TileServer class
 */
class TileServer : ReadWriteLock {
private:
    std::string workDir_;
    std::map<std::string, MapContainer> mapCont_;

public:
    const static std::string srsLlc_;
    const static std::string srsMerc_;

private:
    void registerFonts();
    void prepareTemplate(const std::string &realPath, const std::string& imgPath, std::string& xmlTemplate) const;

public:

    bool loadMap(const std::string& mapFile);
    bool unloadMap(const std::string &mapFile);
    void setWorkDir(const std::string& workDir);
    bool setZoom(const std::string& mapName, int currentZoom);
    mapnik::image_32 getTileBitmap(const std::string& mapName, const std::string& layerId, int x, int y, int z);
    std::string getTileString(const std::string& mapName, const std::string& layerId, int x, int y, int z, bool force, const std::string& ext = "png");
    bool renderIcon(const std::string& mapName, const std::vector<Point>& bbox, std::string& iconData);
public:
    // explicitly disallow copying of this clas
    TileServer& operator=(const TileServer& layer) = delete;
};
