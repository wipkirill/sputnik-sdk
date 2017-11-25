#include "EndPointPlugin.h"

//------------------------------------------------------------------------------
// End point handler
//------------------------------------------------------------------------------
/**
 * @brief EndPointHandler
 * @param outputFile
 * @param edgFilter
 */
EndPointPlugin::EndPointPlugin(const string &outputFile, EdgeFilter *edgFilter):
    outFileName_(),vertices_(),edgeFilter_(0),endPoints_(), edgeSet_(), badEdgeSet_(), junctions_(),
    waysSkipped_(0), endPointMarker_(), maxVertexId_(0), maxEdgeId_(0) {
    edgeFilter_ = edgFilter;
    outFileName_ = outputFile + ".endpoints";

    // set google hash map settings
    endPointMarker_.set_deleted_key(Vertex::NullVertexId);
    vertices_.set_deleted_key(Vertex::NullVertexId);
    endPoints_.set_deleted_key(Vertex::NullVertexId);
    edgeSet_.set_deleted_key(edgeKey(Vertex::NullVertexId, Vertex::NullVertexId));
}
/**
 *
 */
EndPointPlugin::~EndPointPlugin() {
    ;
}
/**
 * @brief initHandler
 */
void EndPointPlugin::init() {
    LOGG(Logger::PROGRESS) << "[ENDPOINT HANDLER] reading end points" << Logger::FLUSH;
}
/**
 * @brief getBadEdgeSet
 * @return
 */
EndPointPlugin::EdgeSet EndPointPlugin::getBadEdgeSet() const {
    return badEdgeSet_;
}
/**
 * @brief getEndPoints
 * @return
 */
EndPointPlugin::VertexSet EndPointPlugin::getEndPoints() const {
    return endPoints_;
}
/**
 * @brief EndPointPlugin::getTurnRestrictions
 * @return
 */
EndPointPlugin::TurnRestrictions EndPointPlugin::getTurnRestrictions() const {
    return restrictions_;
}
/**
 * @brief getVertices
 * @return
 */
EndPointPlugin::VertexSet EndPointPlugin::getVertices() const {
    return vertices_;
}
/**
 * @brief isValidNode
 * @param node
 * @return
 */
bool EndPointPlugin::isValidNode(OSMNode* /*node*/) const {
    return true;
}
/**
 * @brief isValidWay
 * @param way
 * @return
 */
bool EndPointPlugin::isValidWay(OSMWay* w) const {
    return isValid(w);
}
/**
 * @brief isValid
 * @param way
 * @return
 */
bool EndPointPlugin::isValid(OSMWay* w) const {
    // should have at least 2 seen vertices one by one
    bool prev = false;
    for(int i = 0; i < w->nRefs; ++i) {
        bool curr = seenVertex(w->nodeRefs[i]);
        if(prev && curr) {
            return true;
        }
        prev = curr;
    }
    return false;
}
/**
 * @brief isValidRelation
 * @param rel
 * @return
 */
bool EndPointPlugin::isValidRelation(OSMRelation* /*relation*/) const {
    return true;
}
/**
 * @brief cleanupHandler
 */
void EndPointPlugin::cleanupHandler() {
    VertexSet dummy1, dummy2;
    swap(vertices_, dummy1);
    swap(endPoints_, dummy2); 
    remove(outFileName_.c_str());
}
/**
 * helper function to find a vertex
 * @brief findVertex
 * @param id
 * @return
 */
Vertex EndPointPlugin::findVertex(VertexId id) const {
    if(vertices_.find(id) != vertices_.end()) {
        return *vertices_.find(id);
    } else {
        return Vertex();
    }
}
/**
 * @brief EndPointPlugin::maxVertexId
 * @return
 */
Vertex::VertexId EndPointPlugin::maxVertexId() const {
    return maxVertexId_;
}
/**
 * @brief maxEdgeId
 * @return
 */
Edge::EdgeId EndPointPlugin::maxEdgeId() const {
    return maxEdgeId_;
}
/**
 * @brief seenVertex
 * @param id
 * @return
 */
bool EndPointPlugin::seenVertex(VertexId id) const {
    if(vertices_.find(id) != vertices_.end())
        return true;
    return false;
}
/**
 * @brief notifyNode
 * @param node
 */
void EndPointPlugin::notifyNode(OSMNode* n) {
    maxVertexId_ = std::max(maxVertexId_, (Vertex::VertexId)n->nID);
    vertices_.insert(n->nID);
}
/**
 * @brief EndPointPlugin::isEndPoint
 * @param id
 * @return
 */
bool EndPointPlugin::existsPoint(VertexId id) {
    return findVertex(id).getId() != Vertex::NullVertexId;
}
/**
 * @brief notifyWay
 * @param way
 */
void EndPointPlugin::notifyWay(OSMWay* w) {
    maxEdgeId_ = std::max(maxEdgeId_, (Edge::EdgeId)w->nID);
    // check if we have seen all vertices on this way
    if(!isValid(w)) {
        waysSkipped_++;
        return;
    }

    // filter the edge
    if(!edgeFilter_->acceptEdge(w))
        return;

    // if the way is crossing the map border
    // split the way and put the endpoints of split
    // parts into endpoints
    for(int i = 0; i < w->nRefs; i++) {
        VertexId curr = findVertex(w->nodeRefs[i]).getId();
        if(curr != Vertex::NullVertexId) {
            bool endPoint = false;
            if(0 < i && i+1 < w->nRefs) {
                // its either the first, last or a point having
                // a neighbor which doesn't have coordinates
                if(!existsPoint(w->nodeRefs[i-1]) || !existsPoint(w->nodeRefs[i+1]))
                    endPoint = true;
            } else {
                endPoint = true;
            }

            if(endPoint)
                endPointMarker_.insert(curr);
        }
    }

    // find the first vertex that has coordinates (way is not split)
    for(int i = 0, p = -1; i < w->nRefs; i++) {
        VertexId curr = w->nodeRefs[i];
        if(endPointMarker_.find(curr) != endPointMarker_.end()) {
            if(p == -1) {
                assert(existsPoint(curr));
                endPoints_.insert(curr);
            } else {
                VertexId prev = w->nodeRefs[p];
                EdgeKeyType key = edgeKey(prev, curr);

                // resolve different edges for same pair of source and destination
                if(edgeSet_.find(key) != edgeSet_.end()) {
                    badEdgeSet_.insert(key);
                }

                // both endpoints of an edge have coordinates
                assert(existsPoint(prev));
                assert(existsPoint(curr));

                endPoints_.insert(prev);
                endPoints_.insert(curr);
                edgeSet_.insert(key);
            }
            p = i;
        }
        if(existsPoint(curr))
            endPointMarker_.insert(curr);
    }
}
/**
 * @brief notifyRelation
 */
void EndPointPlugin::notifyRelation(OSMRelation* rel) {
    // check if the relation has a turn restriction
    for(int i = 0; i < rel->nTags; i++) {
        string key(rel->tags[i].key), value(rel->tags[i].value);
        if(key == "type" && value == "restriction") {
            string type = "";
            for(int j = 0; j < rel->nTags; j++) {
                string curr(rel->tags[j].key);
                if(curr == "restriction") {
                    type = string(rel->tags[j].value);
                    break;
                }
            }
            if(type != "") {
                Vertex::VertexId via = Vertex::NullVertexId;
                Edge::EdgeId from = Vertex::NullVertexId, to;
                for(int j = 0; j < rel->nMembers; j++) {
                    OSMMemberType mtype = rel->members[j].eType;
                    string role = string(rel->members[j].role);

                    if(mtype == OSMMemberType::MEMBER_WAY) {
                        if(role == "from") {
                            from = rel->members[j].nID;
                        } else if(role == "to") {
                            to = rel->members[j].nID;
                        } else {
                            LOGG(Logger::WARNING) << "Unknown role " << role << Logger::FLUSH;
                        }
                    } else if(mtype == OSMMemberType::MEMBER_NODE) {
                        if(role == "via") {
                            via = rel->members[j].nID;
                        }
                    }
                }

                // make sure the vertex is in our dataset
                if(vertices_.find(via) == vertices_.end()) {
                    continue;
                }

                if(via != Vertex::NullVertexId && from != Edge::NullEdgeId && to != Edge::NullEdgeId) {
                    // determine restriction type

                    TurnRestriction restrict(from, to, via, type);
                    if(restrictions_.find(via) == restrictions_.end()) {
                        restrictions_[via] = {restrict};
                    } else {
                        restrictions_.find(via)->second.push_back(restrict);
                    }
                    // make sure that via is an endpoint
                    endPoints_.insert(via);
                } else {
                    LOGG(Logger::WARNING) << "Not all of via, from, to are present in " << rel->nID << Logger::FLUSH;
                }
            }
        }
    }
}
/**
 * @brief finalize
 */
void EndPointPlugin::finalize() {
    LOGG(Logger::DEBUG) << "[END POINT HANDLER] number of end points " << endPoints_.size() << Logger::FLUSH;
    LOGG(Logger::DEBUG) << "[END POINT HANDLER] number of vertices " << vertices_.size() << Logger::FLUSH;
    LOGG(Logger::DEBUG) << "[END POINT HANDLER] Skipped ways" << waysSkipped_ << Logger::FLUSH;
    FILE * outFile = fopen(outFileName_.c_str(), "w");

    string printFormat = type_spec<int64_t>()+"\n";
    fprintf(outFile, printFormat.c_str(), (int64_t)endPoints_.size());
    for(const Vertex &ver : endPoints_) {
        fprintf(outFile, type_spec<Vertex::VertexId>().c_str(), ver.getId());
        fprintf(outFile, "\n");
    }
    fclose(outFile);
}
