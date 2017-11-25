// Vertices.cpp
//

#include <UrbanLabs/Sdk/GraphCore/Vertices.h>

using namespace std;

//------------------------------------------------------------------------------
// Vertex
//------------------------------------------------------------------------------
const Vertex::VertexId Vertex::NullVertexId = -1;

/**
 * @brief Vertex
 */
Vertex::Vertex() : id_(Vertex::NullVertexId) {
    ;
}

/**
 * @brief Vertex
 * @param id
 */
Vertex::Vertex(VertexId id) : id_(id) {
   ;
}

/**
 * @brief getId
 * @return
 */
Vertex::VertexId Vertex::getId() const {
    return id_;
}

/**
 * @brief setId
 * @param id
 */
void Vertex::setId(VertexId id) {
    id_ = id;
}

/**
 * @brief operator ==
 * @param v
 * @return
 */
bool Vertex::operator == (const Vertex &v) const {
    return v.getId() == this->getId();
}

/**
 * @brief operator !=
 * @param v
 * @return
 */
bool Vertex::operator != (const Vertex &v) const {
    return v.getId() != this->getId();
}
//------------------------------------------------------------------------------
// VertexPoint
//------------------------------------------------------------------------------
/**
 * @brief VertexPoint
 */
VertexPoint::VertexPoint() : pt_(), ver_(0) {
    ;
}

/**
 * @brief VertexPoint
 * @param ver
 * @param pt
 */
VertexPoint::VertexPoint(const Vertex &ver, const Point &pt) : pt_(pt), ver_(ver){
    ;
}

/**
 * @brief VertexPoint
 * @param id
 * @param lat
 * @param lon
 */
VertexPoint::VertexPoint(Vertex::VertexId id, CoordType lat, CoordType lon)
    : pt_(lat, lon), ver_(id) {
    ;
}

/**
 * @brief getId
 * @return
 */
Vertex::VertexId VertexPoint::getId() const {
    return ver_.getId();
}

/**
 * @brief getPoint
 * @return
 */
Point VertexPoint::getPoint() const {
    return pt_;
}

/**
 * @brief getVertex
 * @return
 */
Vertex VertexPoint::getVertex() const {
    return ver_;
}

/**
 * @brief serialize
 * @param oss
 * @return
 */
std::ostream& VertexPoint::serialize(std::ostream& oss) const {
    oss << ver_.getId() << " " << pt_.lat() << " " << pt_.lon() << endl;
    return oss;
}

/**
 * @brief serializeToFile
 * @param file
 */
void VertexPoint::serializeToFile(FILE* file) const {
    string printFormat = type_spec<VertexId>()+StringConsts::SEPARATOR+
            "%.8f"+StringConsts::SEPARATOR+"%.8f\n";
    fprintf(file, printFormat.c_str(), ver_.getId(), pt_.lat(), pt_.lon());
}

/**
 * @brief operator <<
 * @param os
 * @param v
 * @return
 */
std::ostream& operator << (std::ostream& os, const VertexPoint& v) {
    Point pt = v.getPoint();
    os << "{\"id\":"<< v.getId()<< ", \"lon\":" << pt.lon() <<",\"lat\":"<< pt.lat()<<"}";
    return os;
}

/**
 * @brief operator ==
 * @param v1
 * @return
 */
bool VertexPoint::operator == (const Vertex &v1) const {
    return v1.getId() == getId();
}
