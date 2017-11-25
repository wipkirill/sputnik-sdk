#include <UrbanLabs/Sdk/GraphCore/EdgesGTFS.h>

/**
 * @brief EdgeGTFS::EdgeGTFS
 * @param tripId
 * @param nextId
 * @param time
 */
EdgeGTFS::EdgeGTFS(Trip::TripId tripId, VertexGTFS::VertexId /*nextId*/, StopTime::StopTimeDiff /*time*/)
    : tripId_(tripId) {// Edge(nextId, time, Vertex::NullVertexId, 0),  {
    abort();
}
/**
 * @brief EdgeGTFS::getTripId
 * @return
 */
Trip::TripId EdgeGTFS::getTripId() const {
    return tripId_;
}
/**
 * @brief EdgeGTFS::getVia
 * @return
 */
VertexGTFS::VertexId EdgeGTFS::getVia() const {
    return Vertex::NullVertexId;
}
/**
 * @brief EdgeGTFS::getNext
 * @return
 */
VertexGTFS EdgeGTFS::getNext() const {
    return next_;
}
/**
 * @brief EdgeGTFS::getNextId
 * @return
 */
VertexGTFS::VertexId EdgeGTFS::getNextId() const {
    return next_.getId();
}
/**
 * @brief getCost
 */
template <>
EdgeGTFS::DistanceMetric::Metric EdgeGTFS::getCost <EdgeGTFS::DistanceMetric> () const {
    return dist_;
}
/**
 * @brief getCost
 */
template <>
EdgeGTFS::TimeMetric::Metric EdgeGTFS::getCost <EdgeGTFS::TimeMetric> () const {
    return time_;
}
