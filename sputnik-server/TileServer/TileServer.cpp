#include <mutex>
#include <regex>
#include <thread>

#include <google/dense_hash_map>

#include <mapnik/version.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/datasource.hpp>
#include <mapnik/feature_type_style.hpp>
#include <mapnik/datasource_cache.hpp>
#include <mapnik/agg_renderer.hpp>
#include <mapnik/svg/output/svg_renderer.hpp>
#include <mapnik/load_map.hpp>
#include <mapnik/image_util.hpp>

#if MAPNIK_VERSION >= 300000
    #define image_data_32 image_rgba8
    #define image_32 image_rgba8
    #include <mapnik/image.hpp>
    #include <mapnik/image_view_any.hpp>
#else
    #include <mapnik/graphics.hpp>
    #include <mapnik/box2d.hpp>
#endif

#include <TileServer/TileServer.h>
#include <UrbanLabs/Sdk/Utils/FileSystemUtil.h>
#include <UrbanLabs/Sdk/Storage/ConnectionsManager.h>
#include <UrbanLabs/Sdk/Storage/Storage.h>

using namespace std;

// Explanation of multithreading in tileserver:
//   Assumptions
//     - we can render different layers from different threads and not block anything
//     - a single layer can be rendered from any number of threads
//     - map container should not be blocked when any layer is rendered
//     - we can load/unload a map container from a single thread only
//     - layers that are not in the container that is loaded/unloaded should be able to
//       render without blocking others
//     - a map object that is being loaded/unloaded should block all layers that it contains

const string TileServer::srsLlc_ = "+proj=latlong +ellps=WGS84 +datum=WGS84 +no_defs";
const string TileServer::srsMerc_ = "+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0"
                                    "+y_0=0.0 +k=1.0 +units=m +nadgrids=@null +wktext +no_defs +over";

const GoogleProjection Layer::epsg3875_ = GoogleProjection(20);

// this will lock all the locks without a deadlock
void lockAll(std::vector<std::mutex> &locks) {
    if(locks.size() == 1) {
        locks[0].lock();
    } else if(locks.size() == 2) {
        std::lock(locks[0], locks[1]);
    } else if(locks.size() == 3) {
        std::lock(locks[0], locks[1], locks[2]);
    } else if(locks.size() == 4) {
        std::lock(locks[0], locks[1], locks[2], locks[3]);
    } else {
        assert(false);
    }
}

//--------------------------------------------------------------------------------------------------
// Google projection
//--------------------------------------------------------------------------------------------------
GoogleProjection::GoogleProjection(int levels)
    : pixPerDegAtZoom(levels)
    , pixPerRadAtZoom(levels)
    , pixAtZoom(levels)
    , pixDiv2AtZoom(levels)
{
    for (int d = 0, pixPerZoom = Layer::TILE_SIZE; d < levels; d++, pixPerZoom *= 2) {
        pixPerDegAtZoom[d] = pixPerZoom / 360.0;
        pixPerRadAtZoom[d] = pixPerZoom / (2.0 * M_PI);
        pixDiv2AtZoom[d] = { pixPerZoom / 2, pixPerZoom / 2 };
        pixAtZoom[d] = pixPerZoom;
    }
}

/**
 * @brief miniMax
 * @param x
 * @param y
 * @param z
 * @return
 */
double GoogleProjection::miniMax(double x, double y, double z) const
{
    return std::min(std::max(x, y), z);
}

/**
 * @brief fromLatLonToPixel
 * @param latLon
 * @param zoom
 * @return
 */
Pixel GoogleProjection::fromLatLonToPixel(const Point& latLon, int zoom) const
{
    std::pair<int, int> atZoom = pixDiv2AtZoom[zoom];
    int xOffset = round(atZoom.first + latLon.lon() * pixPerDegAtZoom[zoom]);
    double relLat = miniMax(sin(DEG_TO_RAD * latLon.lat()), -0.9999, 0.9999);
    int yOffset = round(atZoom.second - 0.5 * log((1.0 + relLat) / (1.0 - relLat)) * pixPerRadAtZoom[zoom]);
    return { xOffset, yOffset };
}

/**
 * @brief fromPixelToLatLon
 * @param pix
 * @param zoom
 * @return
 */
Point GoogleProjection::fromPixelToLatLon(const Pixel& pix, int zoom) const
{
    std::pair<int, int> atZoom = pixDiv2AtZoom[zoom];
    double lonOffset = (pix.x - atZoom.first) / pixPerDegAtZoom[zoom];
    double relLat = -(pix.y - atZoom.second) / pixPerRadAtZoom[zoom];
    double latOffset = RAD_TO_DEG * (2.0 * atan(exp(relLat)) - 0.5 * M_PI);
    return { latOffset, lonOffset };
}

//--------------------------------------------------------------------------------------------------
// Tile Cache entry
//--------------------------------------------------------------------------------------------------
/**
 * @brief TileCache::TileCacheEntry
 * @param name
 */
TileCacheEntry::TileCacheEntry(const string& nm)
    : name_(nm),  stamp_(std::chrono::system_clock::now())
{
    ;
}
/**
 * @brief TileCacheEntry::operator <
 * @param entry
 * @return
 */
bool TileCacheEntry::operator < (const TileCacheEntry& entry) const
{
    // from c++ docs: priority queues are a type of container
    // adaptors, specifically designed such that its first
    // element is always the greatest of the elements it contains,
    // according to some strict weak ordering criterion

    // so, the greatest is with the smallest time stamp (in past)
    return stamp_ > entry.stamp_;
}
/**
 * @brief TileCacheEntry::getName
 * @return
 */
string TileCacheEntry::getName() const
{
    return name_;
}
//--------------------------------------------------------------------------------------------------
// Memory cache
//--------------------------------------------------------------------------------------------------
TileMemoryCache::TileMemoryCache(int maxSize)
    : maxSize_(maxSize)
{
    ids_.set_deleted_key("-");
    ids_.set_empty_key("");

    double toMb = (1 << 20);
    double avgSize = 40 * (1 << 10);
    int entrySize = sizeof(TileCacheEntry) + sizeof(struct tm);
    LOGG(Logger::INFO) << "Size of tile cache: " << (maxSize * entrySize + avgSize) / toMb << "MB" << Logger::FLUSH;
}

/**
 * @brief putWithContent
 * @param name
 * @param content
 */
void TileMemoryCache::putWithContent(const string& name, const string& content)
{
    ReadWriteLock::WriteLock guard(this);
    if (cache_.size() > maxSize_) {
        auto entry = cache_.top();
        cache_.pop();
        string oldName = entry.getName();
        ids_.erase(oldName);
    }

    Entry newEntry(name);
    cache_.push(newEntry);
    ids_[name] = content;
    cache_.push(name);
}
/**
 * @brief get
 * @param name
 * @param content
 * @return
 */
bool TileMemoryCache::get(const string& name, string& content)
{
    ReadWriteLock::ReadLock guard(this);
    if (ids_.find(name) != ids_.end()) {
        content = ids_[name];
        return true;
    }
    return false;
}
//--------------------------------------------------------------------------------------------------
// External memory cache
//--------------------------------------------------------------------------------------------------
/**
 * @brief TileCache::TileCache
 * @param wd
 */
TileExternalCache::TileExternalCache(int maxSize)
    : maxSize_(maxSize)
{
    ids_.set_empty_key("/");
    ids_.set_deleted_key("//");

    double toMb = (1 << 20);
    int entrySize = sizeof(TileCacheEntry) + sizeof(struct tm);
    LOGG(Logger::INFO) << "Size of tile cache: " << maxSize * entrySize / toMb << "MB" << Logger::FLUSH;
}
/**
 * @brief TileCache::put
 * @param name
 */
void TileExternalCache::put(const string& name)
{
    // delete a cache entry
    ReadWriteLock::WriteLock guard(this);
    if (cache_.size() > maxSize_) {
        TileCacheEntry oldEntry = cache_.top();
        cache_.pop();
        ids_.erase(oldEntry.getName());
        remove(oldEntry.getName().c_str());
    }
    // put the new cache entry
    TileCacheEntry newEntry(name);
    cache_.push(newEntry);
    ids_.insert(name);
}
/**
 * @brief put
 * @param name
 * @param content
 */
void TileExternalCache::putWithContent(const string& name, const string& content)
{
    // delete a cache entry
    ReadWriteLock::WriteLock guard(this);
    if (cache_.size() > maxSize_) {
        TileCacheEntry oldEntry = cache_.top();
        cache_.pop();
        ids_.erase(oldEntry.getName());
        remove(oldEntry.getName().c_str());
    }

    // write the content to file
    // WARNING: this will not overwrite the file content!!
    if (!FsUtil::writeToFileBinary(name, content)) {
        LOGG(Logger::ERROR) << "writeToFileBinary to " << name << " failed" << Logger::FLUSH;
        return;
    }

    // put the new cache entry
    TileCacheEntry newEntry(name);
    cache_.push(newEntry);
    ids_.insert(name);
}
/**
 * @brief TileCache::get
 * @param name
 * @param content
 * @return
 */
bool TileExternalCache::get(const string& name, string& content)
{
    {
        ReadWriteLock::ReadLock guard(this);
        if (ids_.find(name) != ids_.end()) {
            if (!FsUtil::fileExists(name)) {
                LOGG(Logger::WARNING) << "Non existing cache entry requested" << Logger::FLUSH;
            } else {
                // the case when id of the tile is in memory
                if (FsUtil::getFileContent(name, content)) {
                    if (content == "") {
                        LOGG(Logger::ERROR) << "Read empty file content" << Logger::FLUSH;
                    } else {
                        return true;
                    }
                }
            }
        }
    }
    {
        ReadWriteLock::WriteLock guard(this);
        ids_.erase(name);
    }

    return false;
}
//--------------------------------------------------------------------------------------------------
// TileCache
//--------------------------------------------------------------------------------------------------
TileCache::TileCache(const string& workDir)
    : workDir_(workDir)
    , metaCache_(500)
    , metaMemCache_(100)
{
    ;
}
/**
 * @brief TileCache::get
 */
boost::optional<string> TileCache::get(const string& fileName) {
    string content;
    if(!checkInMemory(fileName, content) && !checkOnDisk(fileName, content))
        return boost::none;
    return content;
}
/**
 * @brief TileCache::checkInMemory
 * @param fullPath
 * @param content
 * @return
 */
bool TileCache::checkInMemory(const string& fullPath, string& content)
{
    if (metaMemCache_.get(fullPath, content)) {
        if (content == "") {
            LOGG(Logger::ERROR) << "empty tile :" << fullPath << Logger::FLUSH;
        }
        return true;
    }
    return false;
}
/**
 * @brief TileCache::checkOnDisk
 * @param layerId
 * @param fullPath
 * @param content
 * @return
 */
bool TileCache::checkOnDisk(const string& fullPath, string& content)
{
    if (metaCache_.get(fullPath, content)) {
        if (content == "") {
            LOGG(Logger::ERROR) << "empty tile :" << fullPath << Logger::FLUSH;
        }
        return true;
    }
    return false;
}
/**
 * @brief TileCache::put
 * @param layerId
 * @param fileName
 * @param tile
 */
void TileCache::put(const string& fileName, const string& tile)
{
    string fullPath = FsUtil::makePath({ workDir_, fileName });

    // asynchronously put the tile to the cache
    thread cachingMem(&TileMemoryCache::putWithContent, &metaMemCache_, fullPath, tile);
    cachingMem.detach();

    thread caching(&TileExternalCache::putWithContent, &metaCache_, fullPath, tile);
    caching.detach();
}
/**
 * @brief TileCache::put
 * @param fileName
 */
void TileCache::put(const string& fileName)
{
    string fullPath = FsUtil::makePath({ workDir_, fileName });

    thread caching(&TileExternalCache::put, &metaCache_, fullPath);
    caching.detach();
}
//--------------------------------------------------------------------------------------------------
// Layer
//--------------------------------------------------------------------------------------------------
Layer::Layer(const string& id, const string& workDir, int numMeta, int margin, int numMaps)
    : id_(id)
    , cache_(workDir)
    , workDir_(workDir)
    , mapLocks_(numMaps)
    , NUM_MAPS(numMaps)
    , NUM_META(numMeta)
    , METATILE_MARGIN(margin)
{
    METATILE_SIZE = TILE_SIZE * NUM_META + 2 * METATILE_MARGIN;
}

/**
 * @brief MapContainer::load
 */
void Layer::load(const string& xml)
{
    // WARNING, this is not thread safe and not marked with a lock. In our code
    // we make sure to call it once only per layer
    metaMaps_.clear();
    for (int i = 0; i < NUM_MAPS; i++) {
        metaMaps_.emplace_back(std::move(mapnik::Map(METATILE_SIZE, METATILE_SIZE)));
        mapnik::load_map_string(metaMaps_[i], xml);
    }

    readExisting();
}
/**
 * @brief MapContainer::getBbox
 * @param x
 * @param y
 * @param z
 * @return
 */
mapnik::box2d<double> Layer::getBbox(int x, int y, int z) const
{
    // calculate pixel positions of bottom-left & top-right
    x -= x % NUM_META;
    y -= y % NUM_META;

    //(x,y)
    //  |----|----|  |
    //  |    |tile|  | <- tile margin
    //--|----|----|--|
    //  |    |    |  |
    //--|----|----|--|
    Pixel p0 = { x * TILE_SIZE - METATILE_MARGIN, (y + NUM_META) * TILE_SIZE + METATILE_MARGIN };
    Pixel p1 = {(x + NUM_META) * TILE_SIZE + METATILE_MARGIN, y * TILE_SIZE - METATILE_MARGIN };

    // TODO: conversion is skipped as its assumed that the data is in
    //       (EPSG:4326) coordinate system
    // NOT DONE: Convert to map projection (e.g. mercator co-ords EPSG:900913)
    Point c0 = epsg3875_.fromPixelToLatLon(p0, z);
    Point c1 = epsg3875_.fromPixelToLatLon(p1, z);

    // convert to EPSG:900913
    c0 = Point::toMercator(c0);
    c1 = Point::toMercator(c1);
    Point::CoordType x1, y1, x2, y2;
    x1 = c0.lon(), y1 = c0.lat(), x2 = c1.lon(), y2 = c1.lat();
    return mapnik::box2d<double>(min(x1, x2), min(y1, y2), max(x1, x2), max(y1, y2));
}
/**
 * @brief Layer
 * @param pt
 * @param zoom
 */
TilePos Layer::tilePosForPoint(const Point &pt, int zoom) {
    Pixel px = epsg3875_.fromLatLonToPixel(pt, zoom);
    return {0, 0, 0};
}
/**
 * @brief MapContainer::getBbox
 * @param x
 * @param y
 * @param z
 * @return
 */
mapnik::box2d<double> Layer::getBboxIgnoreMeta(int x, int y, int z) const
{
    // calculate pixel positions of bottom-left & top-right
    Pixel p0 = { x * TILE_SIZE, (y + 1) * TILE_SIZE };
    Pixel p1 = {(x + 1) * TILE_SIZE, y * TILE_SIZE };

    // TODO: conversion is skipped as its assumed that the data is in
    //       (EPSG:4326) coordinate system
    // NOT DONE: Convert to map projection (e.g. mercator co-ords EPSG:900913)
    Point c0 = epsg3875_.fromPixelToLatLon(p0, z);
    Point c1 = epsg3875_.fromPixelToLatLon(p1, z);

    // convert to EPSG:900913
    c0 = Point::toMercator(c0);
    c1 = Point::toMercator(c1);
    Point::CoordType x1, y1, x2, y2;
    x1 = c0.lon(), y1 = c0.lat(), x2 = c1.lon(), y2 = c1.lat();
    return mapnik::box2d<double>(min(x1, x2), min(y1, y2), max(x1, x2), max(y1, y2));
}

mapnik::image_32 Layer::renderTile(int x, int y, int z) {
    // get map id
    int id = getMetaId(x, y, z);
    lock_guard<std::mutex> guard(mapLocks_[id]);

    // zoom map to the tile
    mapnik::image_32 tmp(metaMaps_[id].width(), metaMaps_[id].height());
    metaMaps_[id].zoom_to_box(getBbox(x, y, z));
    mapnik::agg_renderer<mapnik::image_32> ren(metaMaps_[id], tmp);
    ren.apply();

    return tmp;
}
/**
 * @brief MapContainer::cutAndSaveTile
 * @param id
 * @param x
 * @param y
 * @param z
 * @param ext
 * @return
 */
string Layer::renderTile(int x, int y, int z, const string& ext)
{
    // get map id
    int id = getMetaId(x, y, z);
    lock_guard<std::mutex> guard(mapLocks_[id]);

    string fileName = id_ + "-" + to_string(x) + "-" + to_string(y) + "-" + to_string(z) + "." + ext;
    // if(const boost::optional<string> &tile = cache_.get(fileName)) {
    //     LOGG(Logger::INFO) << "Loaded tile " + fileName + " from cache" << Logger::FLUSH;
    //     return tile.get();
    // }

    // render tile
    if (ext == "png") {
        // zoom map to the tile
        mapnik::image_32 tmp(metaMaps_[id].width(), metaMaps_[id].height());
        metaMaps_[id].zoom_to_box(getBbox(x, y, z));
        mapnik::agg_renderer<mapnik::image_32> ren(metaMaps_[id], tmp);
        ren.apply();

        // upper left tile is the base tile
        int xx = x, yy = y;
        x -= x % NUM_META;
        y -= y % NUM_META;

        string content;
        for (int i = x; i < x + NUM_META; i++)
            for (int j = y; j < y + NUM_META; j++) {
                // render metatile
                int height = TILE_SIZE, width = TILE_SIZE;
                int dx = TILE_SIZE * (i - x) + METATILE_MARGIN, dy = TILE_SIZE * (j - y) + METATILE_MARGIN;

#if MAPNIK_VERSION >= 300000
                mapnik::image_view<mapnik::image<mapnik::rgba8_t>> vw1(dx, dy, width, height, tmp);
                struct mapnik::image_view_any vw(vw1);
                //LOGG(Logger::INFO) << "Render data" << width << height << tmp.data() << Logger::FLUSH;
#else
                mapnik::image_view<mapnik::image_data_32> vw(dx, dy, width, height, tmp.data());
#endif
                string tile = mapnik::save_to_string(vw, "png:z=0:e=miniz:s=huff");
                string fileNameMeta = id_ + "-" + to_string(i) + "-" + to_string(j) + "-" + to_string(z) + "." + ext;
                //cache_.put(fileNameMeta, tile);

                if (i == xx && j == yy) {
                    content.swap(tile);
                }
            }
        return content;
    } else if (ext == "svg") {
        // zoom map to the tile
        metaMaps_[id].zoom_to_box(getBboxIgnoreMeta(x, y, z));

        //svg renderer
        ostringstream outStream;
        ostream_iterator<char> output_stream_iterator(outStream);
        mapnik::svg_renderer<ostream_iterator<char> > renderer(metaMaps_[id], output_stream_iterator);
        renderer.apply();

        // cache the tile
        string tile = outStream.str();
        //cache_.put(fileName, tile);
        return tile;
    } else {
        LOGG(Logger::ERROR) << "No extension format" << Logger::FLUSH;
        return "";
    }
}
/**
 * @brief Layer::isValidTileName
 */
bool Layer::isValidTileName(const string &fileName) const {
    // validate the name of the file
    // the format should be of the form <layerId>-x-y-z.ext
    //return std::regex_match(fileName, std::regex("\w+(-\d+){3}\.\w{3}"));
    return false;
}
/**
 *  @brief Layer::extractLayerId
 */
string Layer::extractLayerId(const string &fileName) const {
    auto pos = fileName.find("-");
    if (pos != string::npos) {
        return fileName.substr(pos);
    }
    return "";
}
/**
 * @brief TileCache::readExisting
 */
void Layer::readExisting()
{
    // read present entries to cache
    LOGG(Logger::DEBUG) << "Reading cached entries" << Logger::FLUSH;

    FsDirectory dir;
    vector<string> tileDirs = { workDir_ };
    string cachePath = FsUtil::makePath(tileDirs);
    if (!dir.open(cachePath)) {
        LOGG(Logger::ERROR) << "Cannot open dir " << cachePath << Logger::FLUSH;
        return;
    }

    // validate filenames of the tiles and save them to cache
    int numEntries = 0;
    while (dir.hasNext()) {
        FsFile file;
        if (!dir.readFile(file)) {
            LOGG(Logger::ERROR) << "Cannot open file " << file.getName() << Logger::FLUSH;
            continue;
        }

        if (!file.isDir()) {
            string path = FsUtil::makePath({ workDir_, file.getName() });
            if(isValidTileName(file.getName())) {
                string layerId = extractLayerId(file.getName());
                if (id_ == layerId) {
                    cache_.put(path);
                    numEntries++;
                } else {
                    LOGG(Logger::WARNING) << "Invalid layer:" << layerId << " " << path << Logger::FLUSH;
                }
            } else {
                remove(path.c_str());
                LOGG(Logger::WARNING) << "Invalid filename: " << path << Logger::FLUSH;
            }
        }
        dir.next();
    }
    LOGG(Logger::DEBUG) << "Stored " << numEntries << " entries" << Logger::FLUSH;
}
/**
 * @brief MapContainer::getMetaId
 * @param x
 * @param y
 * @param z
 * @return
 */
int Layer::getMetaId(int /*x*/, int /*y*/, int /*z*/) const {
    // rand() is not a threadsafe function
    static std::mutex lock;
    lock_guard<std::mutex> guard(lock);
    return std::rand() % NUM_MAPS;
}
/**
 * @brief Layer::getName
 * @return
 */
string Layer::getId() const
{
    return id_;
}
//--------------------------------------------------------------------------------------------------
// MapContainer
//--------------------------------------------------------------------------------------------------
/**
 * @brief MapContainer::MapContainer
 * @param name
 */
MapContainer::MapContainer(const string& name)
    : zoom_(DEFAULT_ZOOM)
    , name_(name)
{
    ;
}
/**
 * @brief MapContainer::addLayer
 * @param layer
 */
void MapContainer::addLayer(Layer&& layer)
{
    layers_.emplace(layer.getId(), std::move(layer));
}
/**
 * @brief MapContainer::getTile
 * @param id
 * @param x
 * @param y
 * @param z
 * @return
 */
string MapContainer::getTileString(const string& id, int x, int y, int z, bool force, const string& ext)
{
    ReadWriteLock::ReadLock guard(this);
    string result;
    if (layers_.count(id)) {
        if (force || zoom_ == z) {
            return layers_.find(id)->second.renderTile(x, y, z, ext);
        } else {
            LOGG(Logger::DEBUG) << "Not force or incorrect zoom, ignoring getTile request" << Logger::FLUSH;
        }
    } else {
        LOGG(Logger::ERROR) << "Couldn't find layer " << id << Logger::FLUSH;
    }

    return "";
}

mapnik::image_32 MapContainer::getTileBitmap(const string& id, int x, int y, int z)
{
    ReadWriteLock::ReadLock guard(this);
    string result;
    if (layers_.count(id)) {
        return layers_.find(id)->second.renderTile(x, y, z);
    } else {
        LOGG(Logger::ERROR) << "Couldn't find layer " << id << Logger::FLUSH;
    }

    return mapnik::image_32(Layer::TILE_SIZE, Layer::TILE_SIZE);
}
/**
 * @brief MapContainer::getName
 */
std::string MapContainer::getName() const
{
    return name_;
}
/**
 * @brief MapContainer::setZoom
 * @param zoom
 */
void MapContainer::setZoom(int zoom)
{
    ReadWriteLock::WriteLock guard(this);
    zoom_ = zoom;
}
//--------------------------------------------------------------------------------------------------
// Tile Server
//--------------------------------------------------------------------------------------------------
/**
 * @brief TileServer::registerFonts
 */
void TileServer::registerFonts()
{
    string fontPath = FsUtil::makePath({ workDir_, Folder::COMMON, Folder::FONTS });
    if (!mapnik::freetype_engine::register_fonts(fontPath)) {
        LOGG(Logger::ERROR) << "Couldn't load fonts from " << fontPath << Logger::FLUSH;
        throw exception();
    } else {
        LOGG(Logger::DEBUG) << "Loaded fonts" << Logger::FLUSH;
    }
}
/**
 * @brief setWorkDir
 * Call this only once at startup!
 * Fonts are located in workDir/common/fonts
 * XML template is in workDir/common/data/OSMBrightCustom.xml
 * @param dir
 */
void TileServer::setWorkDir(const string& workDir)
{
    ReadWriteLock::WriteLock guard(this);

    // required directories:
    // - common/data/
    // - common/data/fonts
    // - common/data/input
    // - tiles

    workDir_ = workDir;
    registerFonts();
    LOGG(Logger::DEBUG) << "Setting workdir to tileserver " << workDir_ << Logger::FLUSH;

    vector<string> inputFiles = {}; //{"sqlite.input"};

    // register datasources
    for (const string& input : inputFiles) {
        vector<string> dsPath = { workDir_, Folder::COMMON, Folder::PLUGIN_INPUT, input };
        if (!mapnik::datasource_cache::instance().register_datasource(FsUtil::makePath(dsPath))) {
            LOGG(Logger::ERROR) << "Couldn't register data source: " << input << Logger::FLUSH;
            throw exception();
        } else {
            LOGG(Logger::DEBUG) << "Registered data source: " << input << Logger::FLUSH;
        }
    }
}
/**
 * @brief TileServer::setZoom
 * @param mapName
 * @param currentZoom
 */
bool TileServer::setZoom(const string& mapName, int currentZoom)
{
    // doesn't technically modify the tile server, only the map container
    ReadWriteLock::ReadLock guard(this);

    if (mapCont_.count(mapName) > 0) {
        mapCont_.find(mapName)->second.setZoom(currentZoom);
        LOGG(Logger::DEBUG) << "Setting zoom: " << mapName << " " << currentZoom << Logger::FLUSH;
    } else {
        LOGG(Logger::WARNING) << "No map: " << mapName << " " << currentZoom << Logger::FLUSH;
        return false;
    }

    return true;
}
/**
 * @brief TileServer::loadMap
 * @param mapFile
 */
bool TileServer::loadMap(const string& mapName)
{
    ReadWriteLock::WriteLock guard(this);

    // describe layer properties for the map
    const static map<string, vector<int> > layerProps = { { "background", { 1, 0 } },
                                                          { "road", { 1, 0 } },
                                                          { "label", { 1, 0 } },
                                                          { "h-label", { 1, 0 } } };

    // prepare path for the map file
    string realPath;
    if (!FsUtil::search(mapName, { workDir_ }, realPath)) {
        LOGG(Logger::ERROR) << "Couldn't find mapfile " << mapName << Logger::FLUSH;
        return false;
    }

    // prepare path of images
    LOGG(Logger::DEBUG) << "Initializing map templates" << mapName << Logger::FLUSH;
    vector<string> imgDirs = { workDir_, Folder::COMMON, Folder::DATA, Folder::IMG };
    string imgPath = FsUtil::addTrailingSlash(FsUtil::makePath(imgDirs));

    MapContainer cont(mapName);
    for (const auto& prop : layerProps) {
        // read layer template
        string id = prop.first, xmlTemplate;
        vector<string> path = { workDir_, Folder::COMMON, Folder::DATA, id + ".xml" };
        string templPath = FsUtil::makePath(path);
        if (!FsUtil::getFileContent(templPath, xmlTemplate)) {
            LOGG(Logger::ERROR) << "Failed to read xml template " << id << Logger::FLUSH;
            return false;
        }
        prepareTemplate(realPath, imgPath, xmlTemplate);

        // prepare layer
        Layer layer(id, FsUtil::makePath({ workDir_, Folder::TILES }), prop.second[0], prop.second[1], 1);
        try {
            layer.load(xmlTemplate);
        }
        catch (exception& e) {
            LOGG(Logger::ERROR) << "Failed to load xml" << mapName << e.what() << Logger::FLUSH;
            return false;
        }
        cont.addLayer(std::move(layer));
    }
    mapCont_.emplace(mapName, std::move(cont));
    LOGG(Logger::DEBUG) << "Done initializing map templates" << mapName << Logger::FLUSH;
    return true;
}
/**
 * @brief TileServer::loadMap
 * @param mapFile
 */
bool TileServer::unloadMap(const string& /*mapName*/)
{
    ReadWriteLock::WriteLock guard(this);
    return true;
}
/**
 * @brief TileServer::prepareTemplate
 * @param xmlTemplate
 * @return
 */
void TileServer::prepareTemplate(const string &realPath, const string& imgPath, string& xmlTemplate) const
{
    // setting correct path to sqlite database(map file)
    xmlTemplate = StringUtils::replaceAll(xmlTemplate, "{%mapfile}", realPath);
    // setting valid path to resources(img)
    xmlTemplate = StringUtils::replaceAll(xmlTemplate, "{%imgdir}", imgPath);
}
/**
 * @brief TileServer::getTile
 * @return
 */
string TileServer::getTileString(const string& mapName, const string& layerId, int x, int y, int z, bool force, const string& ext)
{
    ReadWriteLock::ReadLock guard(this);

    if (mapCont_.count(mapName) == 0) {
        LOGG(Logger::DEBUG) << "[RENDERER] No map with name: " << mapName << Logger::FLUSH;
        return "";
    }

    Timer timer;
    LOGG(Logger::DEBUG) << "Start rendering x=" << x << "y=" << y << "z=" << z << "..." << Logger::FLUSH;

    // render tile
    string tile = mapCont_.find(mapName)->second.getTileString(layerId, x, y, z, force, ext);
    if (tile == "") {
        LOGG(Logger::ERROR) << "Empty tile drawn: "
                            << "x=" << x << "y=" << y << "z=" << z << Logger::FLUSH;
    }

    timer.stop();
    LOGG(Logger::DEBUG) << "Done rendering in " << timer.getElapsedTimeSec() << "x=" << x << "y=" << y << "z=" << z << layerId << Logger::FLUSH;
    return tile;
}
/**
 * @brief TileServer::getTile
 * @return
 */
mapnik::image_32 TileServer::getTileBitmap(const string& mapName, const string& layerId, int x, int y, int z)
{
    ReadWriteLock::ReadLock guard(this);

    if (mapCont_.count(mapName) == 0) {
        LOGG(Logger::DEBUG) << "[RENDERER] No map with name: " << mapName << Logger::FLUSH;
        return mapnik::image_32();
    }

    Timer timer;
    LOGG(Logger::DEBUG) << "Start rendering x=" << x << "y=" << y << "z=" << z << "..." << Logger::FLUSH;

    // render tile
    mapnik::image_32 tile = mapCont_.find(mapName)->second.getTileBitmap(layerId, x, y, z);

    timer.stop();
    LOGG(Logger::DEBUG) << "Done rendering in " << timer.getElapsedTimeSec() << "x=" << x << "y=" << y << "z=" << z << layerId << Logger::FLUSH;
    return tile;
}
/**
 * @brief TileServer::renderIcon
 * @param mapFile
 * @return
 */
bool TileServer::renderIcon(const string &realPath, const std::vector<Point> &bbox, string &iconData) {
    ReadWriteLock::ReadLock guard(this);

    // prepare path of images
    LOGG(Logger::DEBUG) << "Initializing map templates" << realPath << Logger::FLUSH;
    vector<string> imgDirs = {workDir_, Folder::COMMON, Folder::DATA, Folder::IMG};

    string imgPath = FsUtil::addTrailingSlash(FsUtil::makePath(imgDirs));

    string id = "background", xmlTemplate;
    string templPath = FsUtil::makePath({ workDir_, Folder::COMMON, Folder::DATA, id + ".xml" });
    if (!FsUtil::getFileContent(templPath, xmlTemplate)) {
        LOGG(Logger::ERROR) << "Failed to read xml template " << id << Logger::FLUSH;
        return false;
    }
    prepareTemplate(realPath, imgPath, xmlTemplate);

    // load map
    mapnik::Map map(Layer::TILE_SIZE, Layer::TILE_SIZE);
    mapnik::load_map_string(map, xmlTemplate);

    Point::CoordType x1 = bbox[0].lon(), y1 = bbox[0].lat(),
                     x2 = bbox[1].lon(), y2 = bbox[1].lat();

    // convert to EPSG:900913
    x1 = Point::lon2x_m(x1), y1 = Point::lat2y_m(y1), x2 = Point::lon2x_m(x2), y2 = Point::lat2y_m(y2);
    mapnik::box2d<double> mbox(min(x1, x2), min(y1, y2), max(x1, x2), max(y1, y2));

    map.zoom_to_box(mbox);
    mapnik::image_32 buf = mapnik::image_32(map.width(), map.height());
    mapnik::agg_renderer<mapnik::image_32> ren(map, buf);
    LOGG(Logger::INFO) << " Start rendering..." << Logger::FLUSH;
    ren.apply();

    iconData = mapnik::save_to_string(buf, "png");
    LOGG(Logger::INFO) << " Done rendering" << Logger::FLUSH;
    return true;
}
