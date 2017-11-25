#pragma once

#include <UrbanLabs/Sdk/GTFS/GTFS.h>
#include <UrbanLabs/Sdk/GraphCore/Vertices.h>

/**
 * @brief The VertexGTFS class
 */
class VertexGTFS : public Vertex {
public:
    typedef Stop::StopId VertexId;
    static const VertexId NullVertexId;
private:
    DateTime date_;
    Trip::TripId prevTripId_;
public:
    VertexGTFS();
    VertexGTFS(const Vertex &v);
    VertexGTFS(Stop::StopId id);
    VertexGTFS(Stop::StopId id, DateTime, Trip::TripId);
    VertexId getId() const;
    bool operator == (const VertexGTFS &v) const;
    bool operator != (const VertexGTFS &v) const;
};

/**
 * @brief The VertexPointGTFS class
 */
class VertexPointGTFS {
public:
    typedef VertexGTFS::VertexId VertexId;
    typedef Point::CoordType CoordType;
private:
    Point pt_;
    VertexGTFS ver_;
public:
    VertexPointGTFS();
    VertexPointGTFS(const VertexGTFS &ver, const Point &pt);
    VertexPointGTFS(VertexGTFS::VertexId id, CoordType lat, CoordType lon);
    VertexId getId() const;
    Point getPoint() const;
    VertexGTFS getVertex() const;
    bool operator == (const Vertex &v1) const;
};

namespace std {
    // specialize hashing function for vertices
    template<>
    class hash<VertexGTFS> {
    public:
        size_t operator()(const VertexGTFS &v) const {
            return hash<VertexGTFS::VertexId>()(v.getId());
        }
    };

    template<>
    class hash<VertexPointGTFS> {
    public:
        size_t operator()(const VertexPointGTFS &v) const {
            return hash<VertexGTFS::VertexId>()(v.getId());
        }
    };
}

/**
 * @brief NearestPointResultGtfs
 */
typedef VertexPointGTFS NearestPointResultGtfs;

