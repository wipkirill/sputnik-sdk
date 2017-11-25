#include "EdgeResolvePlugin.h"
#include <UrbanLabs/Sdk/Storage/SqlConsts.h>

//------------------------------------------------------------------------------
// Edge Resolve plugin
//------------------------------------------------------------------------------
/**
 * @brief EdgeResolveHandler
 * @param outputFile
 * @param edgFilter
 */
EdgeResolvePlugin::EdgeResolvePlugin(const string &outputFile, EdgeFilter *edgFilter)
    : EndPointPlugin(outputFile, edgFilter) {
    pluginId_ = "EDGE_RESOLVER";
    edgeFilter_ = edgFilter;
    outFileName_ = outputFile + ".endpoints";

    // set google hash map settings
    vertices_.set_deleted_key(Vertex::NullVertexId);
    endPoints_.set_deleted_key(Vertex::NullVertexId);
    edgeSet_.set_deleted_key(edgeKey(Vertex::NullVertexId, Vertex::NullVertexId));
    vertexPointFileName_ = outputFile+".vp.sqlite";

    URL url(vertexPointFileName_);
    Properties props = {{"type", "sqlite"},{"table", SqlConsts::VP_TABLE},{"create", "1"}};
    if(!vpIndex_.open(url, props))
        die(pluginId_, "can't initialize vertexPoint database");
}
/**
 * @brief initHandler
 */
void EdgeResolvePlugin::init() {
    LOGG(Logger::PROGRESS) << "[EDGE RESOLVE HANDLER] reading end points" << Logger::FLUSH;
}
/**
 * @brief setBadEdges
 * @param badEdges
 */
void EdgeResolvePlugin::setBadEdges(const EdgeSet &badEdges) {
    badEdgeSet_ = badEdges;
}
/**
 * @brief setEndPoints
 * @param endpoints
 */
void EdgeResolvePlugin::setEndPoints(const VertexSet &endpoints) {
    endPoints_ = endpoints;
}
/**
 * @brief setVertices
 * @param vertices
 */
void EdgeResolvePlugin::setVertices(const VertexSet &vertices) {
    vertices_ = vertices;
}
/**
 * @brief cleanupHandler
 */
void EdgeResolvePlugin::cleanupHandler() {
    VertexSet dummy1, dummy2;
    swap(vertices_, dummy1);
    swap(endPoints_, dummy2);
    remove(vertexPointFileName_.c_str());
}
/**
 * @brief notifyNode
 * @param node
 */
void EdgeResolvePlugin::notifyNode(OSMNode* n) {
    if(!vpIndex_.insertPoint(n->nID, n->dfLat, n->dfLon)) {
        die(pluginId_, "can't insert to vertexPoint database");
    }
}
/**
 * is called to process a way of OSM file
 * @brief way
 * @param way
 */
void EdgeResolvePlugin::notifyWay(OSMWay* w) {
    // check if we have seen all vertices on this way
    if(!isValid(w))
        return;

    // filter the edge
    if(!edgeFilter_->acceptEdge(w))
        return;

    int numEdges = 0;
    VertexId prev = w->nodeRefs[0];

    // marks if the same vertex was seen on the way
    // needed to resolve the self loop case
    VertexSet seen;
    if(existsPoint(w->nodeRefs[0]))
        seen.insert(w->nodeRefs[0]);
    for(int i = 1; i < w->nRefs; i++) {
        numEdges++;
        if(endPoints_.find(w->nodeRefs[i]) != endPoints_.end()) {
            VertexId curr = w->nodeRefs[i];
            EdgeKeyType key = edgeKey(prev, curr);

            // resolve the case when there are multiple edges with the same src and destination
            if(badEdgeSet_.find(key) != badEdgeSet_.end() || seen.find(curr) != seen.end()) {
                // checks if we can split the current edge
                if(numEdges >= 2) {
                    VertexId fix = w->nodeRefs[i-1];
                    if(existsPoint(fix)) {
                        endPoints_.insert(fix);
                    } else {
                        LOGG(Logger::WARNING) << "Cannot fix multiple edges " << key.first << " " << key.second << Logger::FLUSH;
                    }
                } else {
                    LOGG(Logger::WARNING) << "Two edges with the same src and dst " << key.first << " " << key.second << Logger::FLUSH;
                }
            }
            prev = w->nodeRefs[i];
            numEdges = 0;
        }
        if(existsPoint(w->nodeRefs[i]))
            seen.insert(w->nodeRefs[i]);
    }
}
/**
 * @brief EdgeResolvePlugin::notifyRelation
 * @param rel
 */
void EdgeResolvePlugin::notifyRelation(OSMRelation* /*rel*/) {
    ;
}
/**
 * @brief finalize
 */
void EdgeResolvePlugin::finalize() {
    LOGG(Logger::PROGRESS) << "[EDGE RESOLVE HANDLER] number of end points " << endPoints_.size() << Logger::FLUSH;
    FILE * outFile = fopen(outFileName_.c_str(), "w");

    string printFormat = type_spec<int64_t>()+"\n";
    fprintf(outFile, printFormat.c_str(), (int64_t)endPoints_.size());
    for(const Vertex &ver : endPoints_) {
        fprintf(outFile, type_spec<Vertex::VertexId>().c_str(), ver.getId());
        fprintf(outFile, "\n");
    }
    fclose(outFile);
    vpIndex_.buildIndex();
    vpIndex_.close();
}

