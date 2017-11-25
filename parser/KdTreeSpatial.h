#pragma once 

#include <stdexcept>
#include <spatialindex/SpatialIndex.h>

#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/Utils/ThreadSafe.h>
#include <UrbanLabs/Sdk/Utils/Timer.h>
#include <UrbanLabs/Sdk/Utils/Logger.h>
#include <UrbanLabs/Sdk/Storage/Storage.h>
#include <UrbanLabs/Sdk/GraphCore/BoundingBox.h>
#include <UrbanLabs/Sdk/GraphCore/NearestNeighbor.h>

/**
 * @brief The Settings class
 */
class Settings {
public:
    static int const DEFAULT_PAGE_SIZE = 4096;
    static int const DEFAULT_NODE_SIZE = 1000;
    static int const DEFAULT_LEAF_SIZE = 1000;
    static double const DEFAULT_FILL_FACTOR;
    static int const DEFAULT_DIMENSION = 2;
};

/**
 * @brief The BaseSqlInputStream class
 */
class BaseSqlInputStream : public SpatialIndex::IDataStream {
protected:
    SqlStream &m_fin_;
    SpatialIndex::RTree::Data *m_pNext_;
public:
    BaseSqlInputStream(SqlStream &stream);
    virtual ~BaseSqlInputStream();
    virtual SpatialIndex::IData *getNext();
    virtual bool hasNext();
    virtual uint32_t size();
    virtual void rewind();
    virtual void readNextEntry() = 0;
};

/**
 * @brief The BaseBulkFileStream class
 */
class BaseBulkFileStream : public SpatialIndex::IDataStream {
protected:
    std::ifstream m_fin_;
    SpatialIndex::RTree::Data *m_pNext_;
public:
    BaseBulkFileStream(const std::string &inputFile);
    virtual ~BaseBulkFileStream();
    virtual SpatialIndex::IData *getNext();
    virtual bool hasNext();
    virtual uint32_t size();
    virtual void rewind();
    virtual void readNextEntry();
    void close();
private:
    BaseBulkFileStream(const BaseBulkFileStream&);
    BaseBulkFileStream& operator=(const BaseBulkFileStream&){return *this;}
};

/**
 * @brief The BulkFileDataStream class
 */
class BulkVertexPointDataStream : public BaseBulkFileStream {
public:
    BulkVertexPointDataStream(const std::string &inputFile);
    void readNextEntry();
};
/**
 * @brief The BulkFileDataStream class
 */
class BulkBoundBoxDataStream : public BaseBulkFileStream {
public:
    BulkBoundBoxDataStream(const std::string &inputFile);
    void readNextEntry();
};
/**
 * @brief The InputDataStream class
 */
class InputDataStream : public BaseSqlInputStream {
public:
    InputDataStream(SqlStream &stream);
    void readNextEntry();
};

/**
 * @brief The BulkFileDataStream class
 */
class ObjectDataStream : public BaseSqlInputStream {
public:
    ObjectDataStream(SqlStream &stream);
    void readNextEntry();
};
/**
 * @brief The IndexVisitor class
 */
class IndexVisitor : public SpatialIndex::IVisitor {
public:
    typedef Vertex::VertexId VertexId;
    typedef OsmGraphCore::NearestPointResult NearestPointResult;
private:
    SpatialIndex::id_type id_;
    SpatialIndex::Point point_;
    NearestPointResult nearestPointRslt_;
public:
    IndexVisitor();
    NearestPointResult getResult();
    SpatialIndex::id_type getId();
    void visitNode(const SpatialIndex::INode &n);
    void visitData(const SpatialIndex::IData &data);
    void visitData(std::vector <const SpatialIndex::IData *> &v);
};
/**
 * @brief The IndexVisitor class
 */
class BoundingBoxIndexVisitor : public SpatialIndex::IVisitor {
public:
    typedef Vertex::VertexId VertexId;
private:
    std::vector<VertexPoint> results_;
    size_t resultsLimit_;
public:
    BoundingBoxIndexVisitor();
    void emptyResults();
    std::vector<VertexPoint> getResults() const;
    void visitNode(const SpatialIndex::INode &n);
    void visitData(const SpatialIndex::IData &data);
    void visitData(std::vector <const SpatialIndex::IData *> &/*v*/);
};
/**
 * @brief The KdTreeSpatial class
 */
class KdTreeSpatial {
public:
    typedef Point::CoordType CoordType;
    typedef OsmGraphCore::NearestPointResult NearestPointResult;
protected:
    SpatialIndex::ISpatialIndex *rTree_;
    SpatialIndex::IStorageManager *diskfile_;
    std::ofstream bulkStream_;
    std::string bulkFileName_;
    std::string baseName_;
protected:
    KdTreeSpatial &operator = (const KdTreeSpatial &kd);
    KdTreeSpatial(const KdTreeSpatial &kd);
public:
    KdTreeSpatial();
    virtual ~KdTreeSpatial();

    template<typename S>
    bool init(const std::string &dbFile, SqlStream *inputStream);
    bool initForBulkLoad(const std::string &baseName, const std::string &indexFile);

    template<typename S>
    void bulkLoadTreeFile();
    template<typename S>
    void bulkLoadTreeStream(SqlStream &inputStream);

    bool insertBulk(const VertexPoint &vertexPoint,const std::string &sep,const std::string &data = "");
    bool insertBulk(const Vertex::VertexId id, const BoundingBox &bbox, const std::string &sep);
    bool findNearestVertex(const VertexPoint &vertexPoint, NearestPointResult &result) const;
    bool findAllInBoundingBox(const Point &hiLeft, const Point &lowRight, std::vector<VertexPoint> &results) const;

    void removeBulkFile();
    void closeBulk();
    void unload();
};

/**
 *
 */
template<typename S>
bool KdTreeSpatial::init(const std::string &dbFile, SqlStream *inputStream) {
    baseName_ = dbFile + ".index";
    SpatialIndex::id_type indexIdentifier = 1;
    std::ifstream indexStream(baseName_+".dat");
    if(indexStream.good()) {
        LOGG(Logger::DEBUG) << "[KDTREE] Loading from " << baseName_ << Logger::FLUSH;
        diskfile_ = SpatialIndex::StorageManager::loadDiskStorageManager(baseName_);
        rTree_ = SpatialIndex::RTree::loadRTree(*diskfile_, indexIdentifier);
    } else {
        if(inputStream == 0) {
            throw std::runtime_error("No input stream for loading kd-tree");
        }
        LOGG(Logger::DEBUG) << "[KDTREE] Creating new tree in " << baseName_ << Logger::FLUSH;
        bulkLoadTreeStream<S>(*inputStream);
        diskfile_ = SpatialIndex::StorageManager::loadDiskStorageManager(baseName_);
        rTree_ = SpatialIndex::RTree::loadRTree(*diskfile_, indexIdentifier);
    }
    return true;
}
/**
 * @brief KdTreeSpatial::bulkLoadTreeFile
 */
template<typename S>
void KdTreeSpatial::bulkLoadTreeFile() {
    S stream(bulkFileName_);

    // Create and bulk load a new RTree with dimensionality 2, using "file" as
    // the StorageManager and the RSTAR splitting policy.
    SpatialIndex::id_type indexIdentifier;
    SpatialIndex::IStorageManager *diskFile = SpatialIndex::StorageManager::
            createNewDiskStorageManager(baseName_, Settings::DEFAULT_PAGE_SIZE);

    SpatialIndex::StorageManager::IBuffer *file =
            SpatialIndex::StorageManager::createNewRandomEvictionsBuffer(*diskFile, 10, false);

    LOGG(Logger::DEBUG) << "[KDTREE] Bulk loading tree..." << Logger::FLUSH;
    SpatialIndex::ISpatialIndex  *rTree =
            SpatialIndex::RTree::createAndBulkLoadNewRTree(SpatialIndex::RTree::BLM_STR,
                                                           stream, *file,
                                                           Settings::DEFAULT_FILL_FACTOR,
                                                           Settings::DEFAULT_NODE_SIZE,
                                                           Settings::DEFAULT_LEAF_SIZE,
                                                           Settings::DEFAULT_DIMENSION,
                                                           SpatialIndex::RTree::RV_RSTAR,
                                                           indexIdentifier);
    if(rTree != 0)
        delete rTree;
    if(file != 0)
        delete file;
    if(diskFile != 0)
        delete diskFile;
    stream.close();
}
/**
 *
 */
template<typename S>
void KdTreeSpatial::bulkLoadTreeStream(SqlStream &inputStream) {
    S stream(inputStream);
    // Create and bulk load a new RTree with dimensionality 2, using "file" as
    // the StorageManager and the RSTAR splitting policy.
    SpatialIndex::id_type indexIdentifier;
    SpatialIndex::IStorageManager *diskFile =
            SpatialIndex::StorageManager::createNewDiskStorageManager(baseName_, Settings::DEFAULT_PAGE_SIZE);

    SpatialIndex::StorageManager::IBuffer *file =
            SpatialIndex::StorageManager::createNewRandomEvictionsBuffer(*diskFile, 10, false);

    LOGG(Logger::DEBUG) << "[KDTREE] Bulk loading tree..." << Logger::FLUSH;
    SpatialIndex::ISpatialIndex *rTree =
            SpatialIndex::RTree::createAndBulkLoadNewRTree(SpatialIndex::RTree::BLM_STR,
                                                            stream, *file,
                                                            Settings::DEFAULT_FILL_FACTOR,
                                                            Settings::DEFAULT_NODE_SIZE,
                                                            Settings::DEFAULT_LEAF_SIZE,
                                                            Settings::DEFAULT_DIMENSION,
                                                            SpatialIndex::RTree::RV_RSTAR,
                                                            indexIdentifier);
    try {
        if (rTree != 0)
            delete rTree;
    } catch(SpatialIndex::InvalidPageException &e) {
        LOGG(Logger::DEBUG) << "Error while deleting RTree :" << e.what()  << Logger::FLUSH;
    }
    LOGG(Logger::DEBUG) << "Deleting Buffer" << Logger::FLUSH;
    try {
        if (file != 0)
            delete file;
    } catch(SpatialIndex::InvalidPageException e) {
        LOGG(Logger::DEBUG) << "Error while deleting Buffer : " << e.what()  << Logger::FLUSH;
    }
    LOGG(Logger::DEBUG) << "Deleting DiskFile";
    try {
        if (diskFile != 0)
            delete diskFile;
    } catch(SpatialIndex::InvalidPageException e) {
        LOGG(Logger::DEBUG) << "Error while deleting the StorageManager : " << e.what() << Logger::FLUSH;
    }
    LOGG(Logger::DEBUG) << "Finished Destruction" << Logger::FLUSH;
    rTree = 0, file = 0, diskFile = 0;
}
