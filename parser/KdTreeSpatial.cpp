#include <UrbanLabs/Sdk/GraphCore/Vertices.h>
#include <UrbanLabs/Sdk/Utils/StringUtils.h>
#include <UrbanLabs/Sdk/Storage/ConnectionsManager.h>
#include "KdTreeSpatial.h"

using namespace std;

const double Settings::DEFAULT_FILL_FACTOR = 0.7;

//--------------------------------------------------------------------------------------------------
// BaseSqlInputStream
//--------------------------------------------------------------------------------------------------
/**
 * @brief BaseSqlInputStream::BaseSqlInputStream
 * @param stream
 */
BaseSqlInputStream::BaseSqlInputStream(SqlStream &stream) : m_fin_(stream), m_pNext_(0) {;}
/**
 * @brief BaseSqlInputStream::~BaseSqlInputStream
 */
BaseSqlInputStream::~BaseSqlInputStream() {
    if(m_pNext_ != 0) {
        delete m_pNext_;
    }
}
/**
 * @brief BaseSqlInputStream::getNext
 * @return
 */
SpatialIndex::IData *BaseSqlInputStream::getNext() {
    if(m_pNext_ == 0) {
        return 0;
    }

    SpatialIndex::RTree::Data *ret = m_pNext_;
    m_pNext_ = 0;

    if(m_fin_.getNext())
        readNextEntry();
    return ret;
}
/**
 * @brief BaseSqlInputStream::hasNext
 * @return
 */
bool BaseSqlInputStream::hasNext() {
    return (m_pNext_ != 0);
}
/**
 * @brief BaseSqlInputStream::size
 * @return
 */
uint32_t BaseSqlInputStream::size() {
    throw Tools::NotSupportedException("Operation not supported.");
}
/**
 * @brief BaseSqlInputStream::rewind
 */
void BaseSqlInputStream::rewind() {
    throw Tools::NotSupportedException("Operation not supported.");
}
/**
 * @brief BaseSqlInputStream::rewind
 */
void BaseSqlInputStream::readNextEntry() {
    throw Tools::NotSupportedException("Operation not supported in base class.");
}
//--------------------------------------------------------------------------------------------------
// InputDataStream
//--------------------------------------------------------------------------------------------------
/**
 * @brief InputDataStream::InputDataStream
 * @param stream
 */
InputDataStream::InputDataStream(SqlStream &stream) : BaseSqlInputStream(stream) {
    if(m_fin_.getNext())
        readNextEntry();
}
/**
 * @brief InputDataStream::readNextEntry
 */
void InputDataStream::readNextEntry() {
    SpatialIndex::id_type id;
    double coords[2];
    std::string start, end, data;
    m_fin_ >> id >> coords[0] >> coords[1] >> start >> end;

    SpatialIndex::Point pt = SpatialIndex::Point(coords, 2);
    data = start+" "+end;
    SpatialIndex::Region p = SpatialIndex::Region(pt, pt);
    // if Vertex(id) is an endpoint leave data empty
    // otherwise store start_end of parent edge
    if(start.empty()) {
        m_pNext_ = new SpatialIndex::RTree::Data(0, NULL, p, id);
    } else {
        m_pNext_ = new SpatialIndex::RTree::Data(data.size(), reinterpret_cast<byte *>((char *)data.c_str()), p, id);
    }
}
//--------------------------------------------------------------------------------------------------
// ObjectDataStream
//--------------------------------------------------------------------------------------------------
/**
 * @brief ObjectDataStream::ObjectDataStream
 * @param stream
 */
ObjectDataStream::ObjectDataStream(SqlStream &stream) : BaseSqlInputStream(stream) {
    if(m_fin_.getNext())
        readNextEntry();
}
/**
 * @brief ObjectDataStream::readNextEntry
 */
void ObjectDataStream::readNextEntry() {
    SpatialIndex::id_type id;
    double coords[2];
    m_fin_ >> id >> coords[0] >> coords[1];

    SpatialIndex::Point pt = SpatialIndex::Point(coords, 2);
    SpatialIndex::Region p = SpatialIndex::Region(pt, pt);
    // if Vertex(id) is an endpoint leave data empty
    // otherwise store start_end of parent edge

    m_pNext_ = new SpatialIndex::RTree::Data(0, NULL, p, id);
}
//--------------------------------------------------------------------------------------------------
// BaseBulkFileStream
//--------------------------------------------------------------------------------------------------
/**
 * @brief BulkFileDataStream::BulkFileDataStream
 * @param inputFile
 */
BaseBulkFileStream::BaseBulkFileStream(const string &inputFile) : m_fin_(), m_pNext_(0)  {
    m_fin_.open(inputFile.c_str());
    if(!m_fin_)
        throw Tools::IllegalArgumentException("Input file not found.");
}
/**
 * @brief BulkFileDataStream::~BulkFileDataStream
 */
BaseBulkFileStream::~BaseBulkFileStream() {
    if(m_pNext_ != 0) {
        delete m_pNext_;
        m_pNext_ = 0;
    }
}
/**
 * @brief BulkFileDataStream::getNext
 * @return
 */
SpatialIndex::IData *BaseBulkFileStream::getNext() {
    if(m_pNext_ == 0) {
        return 0;
    }
    SpatialIndex::RTree::Data *ret = m_pNext_;
    m_pNext_ = 0;
    if(m_fin_)
        readNextEntry();
    return ret;
}
/**
 * @brief BaseBulkFileStream::hasNext
 * @return
 */
bool BaseBulkFileStream::hasNext() {
    return (m_pNext_ != 0);
}
/**
 * @brief BaseBulkFileStream::close
 */
void BaseBulkFileStream::close() {
    m_fin_.close();
}
/**
 * @brief BaseBulkFileStream::size
 * @return
 */
uint32_t BaseBulkFileStream::size() {
    throw Tools::NotSupportedException("Operation not supported.");
}
/**
 * @brief BaseBulkFileStream::rewind
 */
void BaseBulkFileStream::rewind() {
    throw Tools::NotSupportedException("Operation not supported.");
}
/**
 * @brief BaseBulkFileStream::readNextEntry
 */
void BaseBulkFileStream::readNextEntry() {
    throw Tools::NotSupportedException("Operation not supported.");
}
//--------------------------------------------------------------------------------------------------
// BulkVertexPointDataStream
//--------------------------------------------------------------------------------------------------
/**
 * @brief BulkFileDataStream::BulkFileDataStream
 * @param inputFile
 */
BulkVertexPointDataStream::BulkVertexPointDataStream(const string &inputFile) : BaseBulkFileStream(inputFile) {
    readNextEntry();
}
/**
 * @brief BulkFileDataStream::readNextEntry
 */
void BulkVertexPointDataStream::readNextEntry() {
    SpatialIndex::id_type id;
    double coords[2];
    string start, end, data;

    m_fin_ >> id >> coords[0] >> coords[1] >> start >> end;
    SpatialIndex::Point pt = SpatialIndex::Point(coords, 2);
    data = start+" "+end;

    if(m_fin_.good()) {
        SpatialIndex::Region p = SpatialIndex::Region(pt, pt);
        if(start == "-1") {
            m_pNext_ = new SpatialIndex::RTree::Data(0, NULL, p, id);
        } else {
            m_pNext_ = new SpatialIndex::RTree::Data(data.size(), reinterpret_cast<byte *>((char *)data.c_str()), p, id);
        }
    }
}
//--------------------------------------------------------------------------------------------------
// BulkBoundBoxDataStream
//--------------------------------------------------------------------------------------------------
/**
 * @brief BulkFileDataStream::BulkFileDataStream
 * @param inputFile
 */
BulkBoundBoxDataStream::BulkBoundBoxDataStream(const string &inputFile) : BaseBulkFileStream(inputFile) {
    readNextEntry();
}
/**
 * @brief BulkFileDataStream::readNextEntry
 */
void BulkBoundBoxDataStream::readNextEntry() {
    SpatialIndex::id_type id;
    double hiLeft[2], loRight[2];

    if(m_fin_.good()) {
        m_fin_ >> id >> hiLeft[0] >> hiLeft[1] >> loRight[0] >> loRight[1];

        double reg[2][2] = {{min(hiLeft[0], loRight[0]), min(hiLeft[1], loRight[1])},
                            {max(hiLeft[0], loRight[0]), max(hiLeft[1], loRight[1])}};

        SpatialIndex::Region p = SpatialIndex::Region(reg[0], reg[1], 2);
        m_pNext_ = new SpatialIndex::RTree::Data(0, NULL, p, id);
    }
}
//--------------------------------------------------------------------------------------------------
// IndexVisitor
//--------------------------------------------------------------------------------------------------
/**
 * @brief IndexVisitor::IndexVisitor
 */
IndexVisitor::IndexVisitor()
    : id_(-1), point_(), nearestPointRslt_() {
}
/**
 * @brief IndexVisitor::getResult
 * @return
 */
OsmGraphCore::NearestPointResult IndexVisitor::getResult() {
    return nearestPointRslt_;
}
/**
 * @brief IndexVisitor::getId
 * @return
 */
SpatialIndex::id_type IndexVisitor::getId() {
    return id_;
}
/**
 * @brief IndexVisitor::visitNode
 * @param n
 */
void IndexVisitor::visitNode(const SpatialIndex::INode &/*n*/) {
    ;
}
/**
 * @brief IndexVisitor::visitData
 * @param data
 */
void IndexVisitor::visitData(const SpatialIndex::IData &data) {
    SpatialIndex::IShape *pS;
    data.getShape(&pS);

    byte *pData = 0;
    uint32_t cLen = 0;
    data.getData(cLen, &pData);

    pS->getCenter(point_);
    id_ = data.getIdentifier();
    nearestPointRslt_.setTarget(VertexPoint(id_,Point(point_.getCoordinate(0),point_.getCoordinate(1))));
    // handle the nonendpoint case
    if(pData != 0) {
        string startEnd(reinterpret_cast<char *>(pData),cLen);
        SimpleTokenator st(startEnd, ' ', '\"', true);
        nearestPointRslt_.setStartId(lexical_cast<Vertex::VertexId>(st.nextToken()));
        nearestPointRslt_.setEndId(lexical_cast<Vertex::VertexId>(st.nextToken()));
    }
    // handle the case when the point is an endpoint
    else {
        nearestPointRslt_.setEndId(id_);
        nearestPointRslt_.setStartId(id_);
    }
    delete pS;
    delete[] pData;
}
/**
 * @brief IndexVisitor::visitData
 * @param v
 */
void IndexVisitor::visitData(std::vector <const SpatialIndex::IData *> &/*v*/) {
}
//--------------------------------------------------------------------------------------------------
// BoundingBoxIndexVisitor
//--------------------------------------------------------------------------------------------------
/**
 * @brief BoundingBoxIndexVisitor::BoundingBoxIndexVisitor
 */
BoundingBoxIndexVisitor::BoundingBoxIndexVisitor()
    : results_(), resultsLimit_(1000) {
}
/**
 * @brief BoundingBoxIndexVisitor::visitNode
 * @param n
 */
void BoundingBoxIndexVisitor::visitNode(const SpatialIndex::INode &/*n*/) {
    ;
}
/**
 * @brief BoundingBoxIndexVisitor::visitData
 * @param data
 */
void BoundingBoxIndexVisitor::visitData(const SpatialIndex::IData &data) {
    // count number of ways
    SpatialIndex::IShape *pS;
    data.getShape(&pS);

    byte *pData = 0;
    uint32_t cLen = 0;
    data.getData(cLen, &pData);

    SpatialIndex::Point pt;
    pS->getCenter(pt);
    SpatialIndex::id_type id = data.getIdentifier();

    // put to the result set
    if(results_.size() < resultsLimit_) {
        Point point(pt.getCoordinate(0), pt.getCoordinate(1));
        results_.push_back(VertexPoint(Vertex(id), point));
    }

    delete pS;
    delete[] pData;
}
/**
 * @brief BoundingBoxIndexVisitor::getResults
 * @return
 */
vector<VertexPoint> BoundingBoxIndexVisitor::getResults() const {
    return results_;
}

/**
 * @brief BoundingBoxIndexVisitor::emptyResults
 */
void BoundingBoxIndexVisitor::emptyResults() {
    vector<VertexPoint> dummy;
    swap(results_, dummy);
}

/**
 * @brief BoundingBoxIndexVisitor::visitData
 * @param v
 */
void BoundingBoxIndexVisitor::visitData(std::vector <const SpatialIndex::IData *> &/*v*/) {
}
//--------------------------------------------------------------------------------------------------
// KdTreeSpatial
//--------------------------------------------------------------------------------------------------
/**
 * @brief KdTreeSpatial::KdTreeSpatial
 */
KdTreeSpatial::KdTreeSpatial()
    : rTree_(0), diskfile_(0), bulkStream_(), bulkFileName_(), baseName_() {
}
/**
 * @brief KdTreeSpatial::initForBulkLoad
 * @param baseName
 * @param indexFile
 */
bool KdTreeSpatial::initForBulkLoad(const string &baseName, const string &inputFile) {
    baseName_ = baseName + ".index";
    bulkFileName_ = inputFile;
    ifstream file(bulkFileName_.c_str());
    if(file) {
        LOGG(Logger::INFO) << "[KDTREE] Bulk file aready exists" << Logger::FLUSH;
        return false;
    }
    bulkStream_.open(bulkFileName_);
    bulkStream_.precision(9);
    if(!bulkStream_.is_open()) {
        LOGG(Logger::ERROR) << "[KDTREE] Error opening bulk file" << Logger::FLUSH;
        return false;
    }
    return true;
}
/**
 * @brief KdTreeSpatial::closeBulk
 */
void KdTreeSpatial::closeBulk() {
    bulkStream_.close();
}
/**
 * @brief KdTreeSpatial::removeBulkFile
 */
void KdTreeSpatial::removeBulkFile() {
    std::remove(bulkFileName_.c_str());
}
/**
 * @brief KdTreeSpatial::~KdTreeSpatial
 */
KdTreeSpatial::~KdTreeSpatial() {
    unload();
}
/**
 * @brief KdTreeSpatial::unload
 */
void KdTreeSpatial::unload() {
    if(rTree_ != 0) {
        LOGG(Logger::DEBUG) << "Deleting RTree" << Logger::FLUSH;
        try {
            delete rTree_;
        } catch(SpatialIndex::InvalidPageException &e) {
            LOGG(Logger::DEBUG) << "Error while deleting RTree :" << e.what()  << Logger::FLUSH;
            rTree_ = 0;
        }
        rTree_ = 0;
    }
    if(diskfile_ != 0) {
        LOGG(Logger::DEBUG) << "Deleting DiskFile";
        try {
            delete diskfile_;
        } catch(SpatialIndex::InvalidPageException e) {
            LOGG(Logger::DEBUG) << "Error while deleting the StorageManager : " << e.what() << Logger::FLUSH;
            diskfile_ = 0;
        }
        LOGG(Logger::DEBUG) << "Finished Destruction" << Logger::FLUSH;
        diskfile_ = 0;
    }
    bulkFileName_ = "";
    baseName_ = "";
}
/**
 * @brief KdTreeSpatial::insert
 * @param vertexPoint
 * @param separator
 * @param data
 */
bool KdTreeSpatial::insertBulk(const VertexPoint &vertexPoint, const string &sep, const string &data) {
    SpatialIndex::id_type id = vertexPoint.getId();
    CoordType coords[2] = {vertexPoint.getPoint().lat(), vertexPoint.getPoint().lon()};
    if(!bulkStream_.good()) {
        return false;
    }
    if(data == "") {
        bulkStream_ << id << sep << coords[0] << sep << coords[1] << endl;
    } else {
        bulkStream_ << id << sep << coords[0] << sep
                    << coords[1] << sep << data << endl;
    }
    return true;
}
/**
 * @brief KdTreeSpatial::insertBulk
 * @param bbox
 * @param separator
 * @param data
 * @return
 */
bool KdTreeSpatial::insertBulk(const Vertex::VertexId id, const BoundingBox &bbox, const string &sep) {
    if(!bulkStream_.good()) {
        return false;
    }
    bulkStream_ << id << sep << bbox.getHiLeft().lat() << sep << bbox.getHiLeft().lon() << sep
                << bbox.getLoRight().lat() << sep << bbox.getLoRight().lon() << endl;
    return true;
}
/**
 * @brief KdTreeSpatial::findNearestVertex
 * @param vertexPoint
 * @return
 */
bool KdTreeSpatial::findNearestVertex(const VertexPoint &vertexPoint, NearestPointResult &result) const {
    IndexVisitor visitor;
    CoordType coords[2] = {vertexPoint.getPoint().lat(), vertexPoint.getPoint().lon()};
    SpatialIndex::Point p = SpatialIndex::Point(coords, 2);
#ifdef DEBUG_KDTREE
    timespec t1, t2;
    clock_gettime(CLOCK_REALTIME, &t1);
#endif
    rTree_->nearestNeighborQuery(1, p, visitor);
#ifdef DEBUG_KDTREE
    clock_gettime(CLOCK_REALTIME, &t2);
    LOGG(Logger::INFO) << "[KDTREE TEST] time of nearest point query " << (t2.tv_sec - t1.tv_sec)  +
                       (float)(t2.tv_nsec - t1.tv_nsec) / 1000000000.0 << Logger::FLUSH;
#endif
    if (visitor.getId() == -1)
        return false;
    result = visitor.getResult();
    return true;
}
/**
 * @brief findAllInBoundingBox
 * @param vertexPoint
 * @param results
 * @return
 */
bool KdTreeSpatial::findAllInBoundingBox(const Point &hiLeft, const Point &lowRight, vector<VertexPoint> &results) const {
    BoundingBoxIndexVisitor visitor;
#ifdef DEBUG_KDTREE
    timespec t1, t2;
    clock_gettime(CLOCK_REALTIME, &t1);
#endif

    // usual case when doesn't cross 0th meridian
    if(hiLeft.lon() <= lowRight.lon()) {
        double plow[2] = {min(hiLeft.lat(), lowRight.lat()), min(hiLeft.lon(), lowRight.lon())};
        double phi[2] = {max(hiLeft.lat(), lowRight.lat()), max(hiLeft.lon(), lowRight.lon())};

        SpatialIndex::Region reg(plow, phi, 2);
        rTree_->intersectsWithQuery(reg, visitor);
#ifdef DEBUG_KDTREE
    clock_gettime(CLOCK_REALTIME, &t2);
    LOGG(Logger::INFO) << "[KDTREE TEST] time of nearest point query " << (t2.tv_sec - t1.tv_sec)  +
                       (float)(t2.tv_nsec - t1.tv_nsec) / 1000000000.0 << Logger::FLUSH;
#endif

        results = visitor.getResults();
    } else if(hiLeft.lon() > lowRight.lon()) {
        // crossing 0th meridian, make 2 requests
        LOGG(Logger::WARNING) << "crossing 0th meridian boundary, check bounding box!" << Logger::FLUSH;
        // from large latitude to 180
        double plow1[2] = {min(hiLeft.lat(), lowRight.lat()), max(hiLeft.lon(), lowRight.lon())};
        double phi1[2] = {max(hiLeft.lat(), lowRight.lat()), 180.0};

        SpatialIndex::Region reg1(plow1, phi1, 2);
        rTree_->intersectsWithQuery(reg1, visitor);
#ifdef DEBUG_KDTREE
    clock_gettime(CLOCK_REALTIME, &t2);
    LOGG(Logger::INFO) << "[KDTREE TEST] time of nearest point query " << (t2.tv_sec - t1.tv_sec)  +
                       (float)(t2.tv_nsec - t1.tv_nsec) / 1000000000.0 << Logger::FLUSH;
#endif
        results = visitor.getResults();

        visitor.emptyResults();
        // from 0 to small latutude
        double plow2[2] = {min(hiLeft.lat(), lowRight.lat()), 0.0};
        double phi2[2] = {max(hiLeft.lat(), lowRight.lat()), min(hiLeft.lon(), lowRight.lon())};

        SpatialIndex::Region reg2(plow2, phi2, 2);
        rTree_->intersectsWithQuery(reg2, visitor);
#ifdef DEBUG_KDTREE
    clock_gettime(CLOCK_REALTIME, &t2);
    LOGG(Logger::INFO) << "[KDTREE TEST] time of nearest point query " << (t2.tv_sec - t1.tv_sec)  +
                       (float)(t2.tv_nsec - t1.tv_nsec) / 1000000000.0 << Logger::FLUSH;
#endif

        results.insert(results.end(), visitor.getResults().begin(), visitor.getResults().end());
    }
    return true;
}

