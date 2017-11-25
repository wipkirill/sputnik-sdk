#pragma once

#include "EndPointPlugin.h"
#include "ParserPlugin.h"
#include <UrbanLabs/Sdk/GraphCore/Point.h>
#include <UrbanLabs/Sdk/GraphCore/Vertices.h>
#include <UrbanLabs/Sdk/GraphCore/Edges.h>
#include <UrbanLabs/Sdk/Storage/Storage.h>
#include <UrbanLabs/Sdk/GraphCore/Model.h>
#include <UrbanLabs/Sdk/GraphCore/Graph.h>
#include "EdgeFilter.h"

#include <google/sparse_hash_map>
#include <google/sparse_hash_set>

#include <vector>
#include <string>

namespace std {
    template<>
    class hash<pair<Edge::EdgeId, Edge::EdgeKey> > {
    public:
        size_t operator()(const pair<Edge::EdgeId, Edge::EdgeKey> &k) const {
            return std::hash<Edge::EdgeId>()(k.first) ^ std::hash<Edge::EdgeKey>()(k.second);
        }
    };
}

/**
 * @brief The RoutingPlugin class
 */
class RoutingPlugin : public Plugin {
public:
    typedef Edge::EdgeType EdgeType;
    typedef Vertex VertexType;
    typedef Vertex::VertexId VertexId;
    typedef VertexId VertexRank;
    typedef VertexPoint::CoordType CoordType;
    // do not change this to integral type
    typedef double DistType;
public:
    template<class E>
    class CompareEdge {
    public:
        bool operator () (const E &e1, const E &e2) {
            return e1.getNextId() < e2.getNextId();
        }
    };
    class EqIds {
    public:
        bool operator()(const VertexId id1, const VertexId id2) const {
            return (id1 == id2);
        }
    };
public:
    // translate vertex ids to 0 based and contuguous
    typedef google::sparse_hash_map<VertexId, VertexId, std::hash<VertexId>, EqIds> VertexIdTranslator;
    typedef google::sparse_hash_map<VertexId, Point, std::hash<VertexId>, EqIds> VertexPointMap;
    typedef google::sparse_hash_map<std::pair<Edge::EdgeId, Edge::EdgeKey>, 
                                    VertexPoint, 
                                    std::hash<std::pair<Edge::EdgeId, Edge::EdgeKey> > > DummyEndPointMap;
    typedef google::sparse_hash_set<VertexId, std::hash<Vertex> > VertexSet;
    typedef google::sparse_hash_map<VertexId, vector<EdgeBack>, std::hash<VertexId> > ToTurnRestriction;
    typedef google::sparse_hash_map<VertexId, vector<EdgeForw>, std::hash<VertexId> > FromTurnRestriction;
private:
    // file for storing vertex and edge info
    std::string outputFileName_;
    //file stores vertices
    std::string vericesFileName_;
    // vertex point file name
    std::string vertexPointFileName_;
    //file for storing edges
    std::string edgeFileName_;
    // vertices output file stream
    FILE * vertFileDescr_;
    // edges output file
    FILE * edgeFileDescr_;
    // edge filter
    EdgeFilter *edgeFilter_;
    // Translate node id's to 0
    VertexIdTranslator translator_;
    // Vertex Point index
    VertexToPointIndexSql vpIndex_;
    // nodes that are endpoints of some ways
    // file containing end points
    std::string endPointsFileName_;
    // nodes that are endpoints of some ways
    VertexSet endPoints_;
    // nodes that were created by splitting edges
    DummyEndPointMap dummyEndPoints_;
    // kdtree for routing stores endpoints
    KdTreeSql kdTreeEndPt_;
    // validation
    size_t totalEndpointsWritten_;
    // store non endpoints and turn rest. dummy vertices
    KdTreeSql kdTreeNonEndPt_;
    // validation
    size_t totalNonEndpointsWritten_;
    // Index for geometry
    GeometryIndexSql indexGeometry_;
    // turn restrictions
    EndPointPlugin::TurnRestrictions turns_;
    // edges leading to turn restriction
    ToTurnRestriction edgesToTurn_;
    // edge leading from turn restriction
    FromTurnRestriction edgesFromTurn_;
    // current free endpoint index and way id
    VertexId freeVerId_, freeEdgeId_;
    // for serializing turn restrictions
    std::string turnRestrictionsName_;
    FILE *turnRestrictionsFile_;
    bool hasTurnRestrictions_;
private:
    bool isRestricted(const VertexId v1);
    bool isEndPoint(const Vertex &v) ;
    VertexId newId(VertexId id);
    void createTurnEdges(VertexId v1, VertexId v2,
                         DistType dist, DistType time,
                         Edge::EdgeType type, Edge::EdgeId origId,
                         bool first);
    void serializeEdge(VertexId ver1, VertexId ver2, DistType dist,
                       DistType time, EdgeType type, Edge::EdgeId origId,
                       VertexId via = Vertex::NullVertexId);
    void serializeGeometry(const Vertex &v1, const Vertex &v2,
                           bool fixed, const std::vector<VertexId> &ids,
                           vector<Point> &pts);
    void serializeVertices();
    void serializeTurnRestrictions();
    void createTurnShortcuts();
public:
    RoutingPlugin(const std::string& outputFile, EdgeFilter *edgFilter, bool compress = true);
    virtual ~RoutingPlugin();
    void setTurnRestrictions(const EndPointPlugin::TurnRestrictions &restrictions);
    VertexId nextEndPoint();
    virtual void init();
    virtual void notifyNode(OSMNode*);
    virtual void notifyWay(OSMWay*);
    virtual void notifyRelation(OSMRelation*);
    virtual void finalize();
    virtual void before_nodes();
    virtual void validate();
    virtual void cleanUp();
    virtual std::vector<std::string> getTableNamesToImport() const;
    virtual std::vector<std::string> getSqlToCreateTables() const;
    virtual std::vector<std::string> getOtherSqlCommands() const;
};
