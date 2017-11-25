#pragma once

#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/GTFS/GTFS.h>
#include <UrbanLabs/Sdk/GraphCore/VerticesGTFS.h>
#include <UrbanLabs/Sdk/GraphCore/Edges.h>

/**
 * @brief The EdgeGTFS class
 */
class EdgeGTFS : public Edge {
public:
    class TimeMetric {
    public:
        typedef EdgeGTFS::EdgeTime Metric;
    };
    class DistanceMetric {
    public:
        typedef EdgeDist Metric;
    };
public:
    typedef int EdgeType;
    typedef StopTime::StopTimeDiff EdgeDist;
private:
    Trip::TripId tripId_;
public:
    EdgeGTFS(Trip::TripId tripId, VertexGTFS::VertexId nextId, StopTime::StopTimeDiff time);
    Trip::TripId getTripId() const;
    VertexGTFS::VertexId getVia() const;
    VertexGTFS getNext() const;
    VertexGTFS::VertexId getNextId() const;
    template <typename Metric>
    typename Metric::Metric getCost() const;
};
