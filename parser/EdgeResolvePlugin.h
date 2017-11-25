#pragma once

#include "EndPointPlugin.h"

/**
 * @brief The EdgeResolveHandler class
 */
class EdgeResolvePlugin : public EndPointPlugin {
private:
    // vertex-point database
    VertexToPointIndexSql vpIndex_;
    std::string vertexPointFileName_;
public:
    EdgeResolvePlugin(const string &outputFile, EdgeFilter *edgFilter);
    void setBadEdges(const EdgeSet &badEdges);
    void setEndPoints(const VertexSet &endpoints);
    void setVertices(const VertexSet &vertices);
public:
    void cleanupHandler();
public:
    virtual void init();
    virtual void notifyNode(OSMNode* node);
    virtual void notifyWay(OSMWay* way);
    virtual void notifyRelation(OSMRelation* /*rel*/);
    virtual void finalize();
};
