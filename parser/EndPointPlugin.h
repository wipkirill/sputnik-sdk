#pragma once

#include <stddef.h>
#include <google/sparse_hash_map>
#include <google/sparse_hash_set>

#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include "EdgeFilter.h"
#include <UrbanLabs/Sdk/Utils/Types.h>
#include <UrbanLabs/Sdk/Storage/Storage.h>
#include "OsmInput.h"
#include <UrbanLabs/Sdk/GraphCore/TurnRestriction.h>
#include "OsmTypes.h"

/**
 * @brief The EndPointHandler class
 */
class EndPointPlugin :  public Plugin, public OsmValidator {
public:
    typedef Vertex VertexType;
    typedef Vertex::VertexId VertexId;
    typedef Edge::EdgeKey EdgeKeyType;
    typedef VertexId VertexRank;
    typedef VertexPoint::CoordType CoordType;
    typedef float DistType;
public:
    class EqIds {
    public:
        bool operator()(const VertexId id1, const VertexId id2) const {
            return (id1 == id2);
        }
    };
    class EqEdgeKeys {
    public:
        bool operator()(const EdgeKeyType id1, const EdgeKeyType id2) const {
            return (id1 == id2);
        }
    };
public:
    // translate vertex ids to 0 based and contuguous
    typedef google::sparse_hash_map<VertexId, std::vector<TurnRestriction>, std::hash<VertexId>, EqIds> TurnRestrictions;
    typedef google::sparse_hash_set<EdgeKeyType, std::hash<EdgeKeyType>, EqEdgeKeys> EdgeSet;
    typedef google::sparse_hash_map<VertexId, VertexId, std::hash<VertexId>, EqIds> VertexIdTranslator;
    typedef google::sparse_hash_map<VertexId, Point, std::hash<VertexId>, EqIds> VertexPointMap;
    typedef google::sparse_hash_set<VertexId, std::hash<Vertex> > VertexSet;
protected:
    // output file
    std::string outFileName_;
    // graph vertices
    VertexSet vertices_;
    // edge filter
    EdgeFilter *edgeFilter_;
    // nodes that are endpoints of some ways
    VertexSet endPoints_;
    // edges encountered
    EdgeSet edgeSet_;
    // edges to be resolved (inserted additional endpoint)
    EdgeSet badEdgeSet_;
private:
    // junctions that have turn restrictions
    VertexSet junctions_;
    // number of invalid ways(one of nodes is not processed by node())
    size_t waysSkipped_;
    // end point marker
    VertexSet endPointMarker_;
    // turn restrictions
    TurnRestrictions restrictions_;
    // max vertex id
    Vertex::VertexId maxVertexId_;
    // max edge id
    Edge::EdgeId maxEdgeId_;
public:
    EndPointPlugin(const std::string &outputFile, EdgeFilter *edgFilter);
    virtual ~EndPointPlugin();
public:
    // methods from OsmValidator
    EdgeSet getBadEdgeSet() const;
    VertexSet getEndPoints() const;
    TurnRestrictions getTurnRestrictions() const;
    virtual VertexSet getVertices() const;
    virtual bool isValidNode(OSMNode*) const;
    virtual bool isValidWay(OSMWay* w) const;
    virtual bool isValidRelation(OSMRelation* /*relation*/) const;
    virtual Vertex findVertex(VertexId id) const;
    virtual Vertex::VertexId maxVertexId() const;
    virtual Edge::EdgeId maxEdgeId() const;
public:
    // methods from Plugin
    virtual void init();
    virtual void notifyNode(OSMNode* n);
    virtual void notifyWay(OSMWay* w);
    virtual void notifyRelation(OSMRelation* rel);
    virtual void finalize();
public:
    bool isValid(OSMWay* w) const;
    bool seenVertex(VertexId id) const;
    void cleanupHandler();
protected:
    bool existsPoint(VertexId id);
};
