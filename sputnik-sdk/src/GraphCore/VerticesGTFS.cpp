#include <UrbanLabs/Sdk/GraphCore/VerticesGTFS.h>
//------------------------------------------------------------------------------
// VertexPoint
//------------------------------------------------------------------------------
/**
 * @brief VertexGTFS::VertexGTFS
 */
VertexGTFS::VertexGTFS() {;}
/**
 * @brief VertexGTFS::VertexGTFS
 * @param v
 */
VertexGTFS::VertexGTFS(const Vertex &v) : Vertex(v.getId()) {;}
/**
 * @brief VertexGTFS::VertexGTFS
 * @param id
 */
VertexGTFS::VertexGTFS(Stop::StopId id) : Vertex(id) {;}
/**
 * @brief VertexGTFS::VertexGTFS
 * @param id
 * @param date_
 * @param prevTripId_
 */
VertexGTFS::VertexGTFS(Stop::StopId id, DateTime /**/, Trip::TripId /**/)
    : Vertex(id) {;}
/**
 * @brief VertexGTFS::getId
 * @return
 */
VertexGTFS::VertexId VertexGTFS::getId() const {
    return id_;
}
/**
 * @brief VertexGTFS::operator ==
 * @param v
 * @return
 */
bool VertexGTFS::operator == (const VertexGTFS &v) const {
    return v.getId() == this->getId();
}
/**
 * @brief VertexGTFS::operator !=
 * @param v
 * @return
 */
bool VertexGTFS::operator != (const VertexGTFS &v) const {
    return v.getId() != this->getId();
}
//------------------------------------------------------------------------------
// VertexPointGTFS
//------------------------------------------------------------------------------
/**
 * @brief VertexPoint
 */
VertexPointGTFS::VertexPointGTFS() : pt_(), ver_(0) {
    ;
}

/**
 * @brief VertexPoint
 * @param ver
 * @param pt
 */
VertexPointGTFS::VertexPointGTFS(const VertexGTFS &ver, const Point &pt) : pt_(pt), ver_(ver) {;}

/**
 * @brief VertexPoint
 * @param id
 * @param lat
 * @param lon
 */
VertexPointGTFS::VertexPointGTFS(VertexGTFS::VertexId id, CoordType lat, CoordType lon)
    : pt_(lat, lon), ver_(id) {
    ;
}

/**
 * @brief getId
 * @return
 */
VertexGTFS::VertexId VertexPointGTFS::getId() const {
    return ver_.getId();
}

/**
 * @brief getPoint
 * @return
 */
Point VertexPointGTFS::getPoint() const {
    return pt_;
}

/**
 * @brief getVertex
 * @return
 */
VertexGTFS VertexPointGTFS::getVertex() const {
    return ver_;
}

/**
 * @brief operator ==
 * @param v1
 * @return
 */
bool VertexPointGTFS::operator == (const Vertex &v1) const {
    return v1.getId() == getId();
}
