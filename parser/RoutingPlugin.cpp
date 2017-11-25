#include <UrbanLabs/Sdk/GraphCore/Point.h>
#include "EndPointPlugin.h"
#include "RoutingPlugin.h"

using std::string;

/**
 * @brief RoutingPlugin::RoutingPlugin
 * @param outputFile
 * @param edgFilter
 */
RoutingPlugin::RoutingPlugin(const string& outputFile, EdgeFilter *edgFilter, bool compress):
    Plugin(), outputFileName_(),vericesFileName_(), vertexPointFileName_(),
    edgeFileName_(),vertFileDescr_(0), edgeFileDescr_(0),
    edgeFilter_(0),translator_(), endPointsFileName_(), endPoints_(), kdTreeEndPt_(),
    totalEndpointsWritten_(0), kdTreeNonEndPt_(), totalNonEndpointsWritten_(0),
    indexGeometry_(),hasTurnRestrictions_(false)
{
    outputFileName_ = outputFile;
    edgeFilter_ = edgFilter;
    pluginId_ = "ROUTING_PLUGIN";
    turnRestrictionsName_ = outputFile+StringConsts::PT+SqlConsts::TURN_RESTRICTIONS;
    turnRestrictionsFile_ = fopen(turnRestrictionsName_.c_str(), "a");
    //write vertices to kdtree
    if(!kdTreeEndPt_.openForBulkLoad(outputFileName_, SqlConsts::KDTREE_ENDPT_TABLE, false)) {
        die(pluginId_, "can't initialize kdTree endpoints");
    }
    if(!kdTreeNonEndPt_.openForBulkLoad(outputFileName_, SqlConsts::KDTREE_NON_ENDPOINT_TABLE, true)) {
        die(pluginId_, "can't initialize kdTree non endpoints");
    }
    if(!compress) {
        indexGeometry_.setCompression(compress);
    }
}
/**
 * @brief RoutingPlugin::~RoutingPlugin
 */
RoutingPlugin::~RoutingPlugin() {
    ;
}
/**
 * @brief isRestricted
 * @param v1
 * @return
 */
bool RoutingPlugin::isRestricted(const VertexId v1) {
    return turns_.find(v1) != turns_.end();
}
/**
 * @brief isEndPoint
 * @param v
 * @return
 */
bool RoutingPlugin::isEndPoint(const Vertex &v) {
    VertexSet::iterator it = endPoints_.find(v.getId());
    if(it != endPoints_.end())
        return true;
    return false;
}
/**
 * returns the translated id of a vertex
 * @brief newId
 * @param id
 * @return
 */
RoutingPlugin::VertexId RoutingPlugin::newId(VertexId id) {
    auto it = translator_.find(id);
    if(it == translator_.end()) {
        die(pluginId_, "can't find point "+lexical_cast(id)+" in vertex id translator");
        return VertexType::NullVertexId;
    } else {
        return it->second;
    }
}
/**
 * @brief RoutingPlugin::setTurnRestrictions
 * @param restirctions
 */
void RoutingPlugin::setTurnRestrictions(const EndPointPlugin::TurnRestrictions &restrictions) {
    turns_ = restrictions;
    hasTurnRestrictions_ = true;
}
/**
 * @brief RoutingPlugin::nextEndPoint
 * @param vpt
 * @return
 */
Vertex::VertexId RoutingPlugin::nextEndPoint() {
    // insert new id
    Vertex::VertexId newId = freeVerId_++;
    return newId;
}
/**
 * This function works only on endpoints!!!
 * @brief RoutingPlugin::createTurnEdges
 * @param v1
 * @param via
 * @param v2
 * @param dist
 * @param time
 * @param origId
 */
void RoutingPlugin::createTurnEdges(VertexId v1, VertexId v2,
                                    DistType dist, DistType time,
                                    Edge::EdgeType type, Edge::EdgeId origId,
                                    bool first) {
    if(first) {
        // v1 is an endpoint here as its restricted
        EdgeForw from(v2, dist, time, Vertex::NullVertexId, type, origId);
        if(edgesFromTurn_.find(v1) != edgesFromTurn_.end())
            edgesFromTurn_[v1].push_back(from);
        else
            edgesFromTurn_[v1] = {from};

        if(!(type & Edge::ONE_WAY)) {
            EdgeBack to(v2, dist, time, Vertex::NullVertexId, type, origId);
            if(edgesToTurn_.find(v1) != edgesToTurn_.end())
                edgesToTurn_[v1].push_back(to);
            else
                edgesToTurn_[v1] = {to};
        }
    } else {
        EdgeBack to(v1, dist, time, Vertex::NullVertexId, type, origId);
        if(edgesToTurn_.find(v2) != edgesToTurn_.end())
            edgesToTurn_[v2].push_back(to);
        else
            edgesToTurn_[v2] = {to};

        // v2 is an endpoint here
        if(!(type & Edge::ONE_WAY)) {
            EdgeForw from(v1, dist, time, Vertex::NullVertexId, type, origId);
            if(edgesFromTurn_.find(v2) != edgesFromTurn_.end())
                edgesFromTurn_[v2].push_back(from);
            else
                edgesFromTurn_[v2] = {from};
        }
    }
}

/**
 * This function works only on endpoints!!!
 * The function throws out turns which create a turn restriction
 * @brief createTurnShortcuts
 */
void RoutingPlugin::createTurnShortcuts() {
    for(auto turn : turns_) {
        // the via that we got from endpoint handler is not translated
        // to endpoint id
        int shortCuts = 0;
        VertexId via = newId(turn.first);

        // get all the turn restrictions at the node
        // and divide them into permissive and restrictive types
        map<Edge::EdgeId, Edge::EdgeId> only;
        map<Edge::EdgeId, set<Edge::EdgeId> > no;
        for(int i = 0; i < turn.second.size(); i++) {
            TurnRestriction curr = turn.second[i];
            if(edgeFilter_->isNoRestriction(curr.getType())) {
                auto found = no.find(curr.getFrom());
                if(found == no.end())
                    no[curr.getFrom()] = {curr.getTo()};
                else
                    found->second.insert(curr.getTo());
            }

            if(edgeFilter_->isOnlyRestriction(curr.getType())) {
                if(only.count(curr.getFrom()))
                    LOGG(Logger::ERROR) << "Contradicting turn restriction for from:" <<
                                           curr.getFrom() << " to:" << curr.getTo() << " via: " <<
                                           curr.getVia() << " type:" << curr.getType() << Logger::FLUSH;
                only[curr.getFrom()] = curr.getTo();
            }
        }

        for(const EdgeForw &from : edgesFromTurn_[via]) {
            for(const EdgeBack &to : edgesToTurn_[via]) {
                // check that the edges do not create a turn restriction
                Edge::EdgeId fId = to.getOrigId(), tId = from.getOrigId();

                // there should be no "no" restriction at the turn
                // if there is a "only" retriction at the turn, it has to be the
                // destination edge
                bool haveNo = no.count(fId) && no[fId].count(tId);
                bool haveOnly = only.count(fId) == 0 || only[fId] == tId;
                if(!haveNo && haveOnly) {
                    // make sure the type of the new edge makes sense
                    Edge::EdgeType type = from.getType() | to.getType() | Edge::TURN_RESTRICTED | Edge::FINAL;
                    if(type & Edge::GEOMETRY_STORED) {
                        // geometry will not be stored in the database
                        type = type ^ Edge::GEOMETRY_STORED;
                    }

                    // create a new shortcut edge
                    DistType time = from.getCost<Edge::TimeMetric>()+to.getCost<Edge::TimeMetric>();
                    DistType dist = from.getCost<Edge::DistanceMetric>()+to.getCost<Edge::DistanceMetric>();
                    Edge::EdgeId origId = from.getOrigId() == to.getOrigId() ? from.getOrigId() : Edge::NullEdgeId;

                    // serialize new edge, no need to serialize geometry
                    serializeEdge(to.getNextId(), from.getNextId(), dist, time, type, origId, via);
                    shortCuts++;
                }
            }
        }
        if(shortCuts == 0) {
            LOGG(Logger::WARNING) << "Vertex " << turn.first << " has 0 shortcuts created" << Logger::FLUSH;
        }
    }
}

/**
 * @brief RoutingPlugin::serializeEdge
 * @param ver1
 * @param ver2
 * @param dist
 * @param time
 * @param type
 * @param origId
 */
void RoutingPlugin::serializeEdge(VertexId ver1, VertexId ver2, DistType dist,
                                  DistType time, EdgeType type, Edge::EdgeId origId,
                                  VertexId via) {
    assert(dist >= 0 && time >= 0);
    EdgeForw edgFrom(ver2, dist, time, via, type, origId);
    string printFormat = type_spec<VertexId>()+StringConsts::SEPARATOR;
    fprintf(edgeFileDescr_, printFormat.c_str(), ver1);
    edgFrom.serializeToFile(edgeFileDescr_);
}
/**
* @brief serializeVertices and fill index
*/
void RoutingPlugin::serializeVertices() {
    string noStartEnd = "", sep = StringConsts::SEPARATOR;
    for(const Vertex &v: endPoints_) {
        Point pt;
        if(!vpIndex_.getPoint(v, pt))
            die(pluginId_, "can't find point in vertexPoint database");
        VertexPoint vpt(newId(v.getId()), pt);
        vpt.serializeToFile(vertFileDescr_);
        if(!kdTreeEndPt_.insertBulk(vpt, sep, noStartEnd))
            die(pluginId_,"can't insert point to kdTree");
        totalEndpointsWritten_++;
    }

    // these were created when resolving turn restrictions by splitting edges
    for(auto &edge : dummyEndPoints_) {
        VertexPoint vpt = edge.second;
        vpt.serializeToFile(vertFileDescr_);
        if(!kdTreeEndPt_.insertBulk(vpt, sep, noStartEnd))
            die(pluginId_,"can't insert point to kdTree");
        totalEndpointsWritten_++;
    }
}
/**
 * @brief serializeTurnRestrictions
 */
void RoutingPlugin::serializeTurnRestrictions() {
    for(const auto &turn : turns_) {
        for(const TurnRestriction &restr : turn.second) {
            // serialize turn restriction
            string sep = StringConsts::SEPARATOR;
            string pSpec = type_spec<Edge::EdgeId>()+sep+type_spec<Edge::EdgeId>()+sep+
                    type_spec<Vertex::VertexId>()+sep+type_spec<string>()+"\n";

            fprintf(turnRestrictionsFile_, pSpec.c_str(), restr.getFrom(), restr.getTo(),
                    newId(restr.getVia()), restr.getType().c_str());
        }
    }
}

/**
 * @brief serializeGeometry
 * @param oss
 * @return
 */
void RoutingPlugin::serializeGeometry(const Vertex &v1, const Vertex &v2,
                                      bool fixed, const vector<VertexId> &ids,
                                      vector<Point> &pts) {
    string sep = StringConsts::SEPARATOR;
    string startEnd = lexical_cast(v1.getId())+" "+lexical_cast(v2.getId());

    for(size_t i = 0; i < ids.size(); i++) {
        // put to kdTree
        VertexPoint vpt(Vertex(ids[i]), pts[i]);
        if(!kdTreeNonEndPt_.insertBulk(vpt, sep, startEnd))
            die(pluginId_,"can't insert point to kdTree");
        totalNonEndpointsWritten_++;
    }
    if(pts.size() > 0) {
        if(!indexGeometry_.insertGeometry(v1.getId(), v2.getId(), fixed, pts))
            die(pluginId_, "can't insert geometry to database");
    }
}
/**
 * @brief RoutingPlugin::init
 * @param outputFile
 * @param edgFilter
 * @param nodesProcessed
 */
void RoutingPlugin::init() {
    // advice to prevent any caching during file read
    vericesFileName_ = outputFileName_ + StringConsts::PT + SqlConsts::VERTICES_TABLE;
    vertFileDescr_ = fopen(vericesFileName_.c_str(), "a");

    //initialize edges file
    edgeFileName_ = outputFileName_ + StringConsts::PT + SqlConsts::EDGES_TABLE;
    edgeFileDescr_ = fopen(edgeFileName_.c_str(),"a");
    endPointsFileName_ = outputFileName_+".endpoints";
    {
        // store geometries in bulk file
        URL url(outputFileName_);
        Properties props = {{"type", "sqlite"}, {"create", "1"}};
        if(!indexGeometry_.open(url, props)) {
            die(pluginId_, "can't initialize geometry database");
        }
    }
    vertexPointFileName_ = outputFileName_+".vp.sqlite";

    // set google hash map settings
    endPoints_.set_deleted_key(Vertex::NullVertexId);
    translator_.set_deleted_key(Vertex::NullVertexId);

    before_nodes();
}
/**
 * @brief before_nodes called before nodes parsing
 */
void RoutingPlugin::before_nodes() {
    URL url(vertexPointFileName_);
    Properties props = {{"type", "sqlite"}, {"table", SqlConsts::VP_TABLE}};
    if(!vpIndex_.open(url, props))
        die(pluginId_, "can't initialize vertexPoint database");

    // unserialize number of points
    int64_t numEndPoints;
    FILE* endPointDescriptor = fopen(endPointsFileName_.c_str(),"r");
    if(fscanf(endPointDescriptor, type_spec<int64_t>().c_str(), &numEndPoints) != 1) {
        LOGG(Logger::ERROR) << "[END POINTS] couldn't read endpoints file" << Logger::FLUSH;
        exit(EXIT_FAILURE);
    }

    // unserialize endpoints
    LOGG(Logger::DEBUG) << "[END POINTS] number of endpoints: " << numEndPoints << Logger::FLUSH;
    for(size_t i = 0; i < numEndPoints; i++) {
        VertexId id;
        int scanned = fscanf(endPointDescriptor, type_spec<VertexId>().c_str(), &id);
        if(scanned != 1) {
            LOGG(Logger::ERROR) << "[END POINTS] couldn't read vertex id on line: " << i+2 << Logger::FLUSH;
            exit(EXIT_FAILURE);
        }
        endPoints_.insert(id);
    }

    LOGG(Logger::DEBUG) << "[END POINTS] loaded endpoints" << Logger::FLUSH;
    fclose(endPointDescriptor);

    // from this point on noone should insert into endpoints
    freeVerId_ = numEndPoints;
    freeEdgeId_ = osmValidator_->maxEdgeId()+1;

    // assign new id's to vertices
    LOGG(Logger::DEBUG) << "[TRANSLATING IDS] size of endpoints: " << endPoints_.size() << Logger::FLUSH;
    VertexId currId = 0;
    for(const Vertex &ver : endPoints_) {
        translator_[ver.getId()] = currId;
        currId++;
    }
}
/**
 * @brief RoutingPlugin::notifyNode
 * @param n
 */
void RoutingPlugin::notifyNode(OSMNode* /*n*/) {
    ;
}
/**
 * @brief notifyRelation
 * @param rel
 */
void RoutingPlugin::notifyRelation(OSMRelation* /*rel*/) {
    ;
}
/**
 * @brief finalize
 */
void RoutingPlugin::finalize() {
    LOGG(Logger::INFO) << "[CREATE TURN SHORTCUTS]" << Logger::FLUSH;
    createTurnShortcuts();
    LOGG(Logger::INFO) << "[SERIALIZE TURN RESTRICTIONS]" << Logger::FLUSH;
    serializeTurnRestrictions();
    LOGG(Logger::INFO) << "[SERIALIZE VERTICES START]" << Logger::FLUSH;
    serializeVertices();
    LOGG(Logger::INFO) << "[SERIALIZE VERTICES END]" << Logger::FLUSH;
    indexGeometry_.close();
    kdTreeEndPt_.closeBulk();
    vpIndex_.close();
    if(vertFileDescr_ != 0)
        fclose(vertFileDescr_);
    if(edgeFileDescr_ != 0)
        fclose(edgeFileDescr_);
    if(turnRestrictionsFile_ != 0)
        fclose(turnRestrictionsFile_);
}
/** 
 * removes consecutive same elements from vectors
 */
void fixWay(vector<Vertex::VertexId> &ids, vector<Point> &pts, vector<int8_t> &found) {
    vector<Point> npts; 
    vector<int8_t> nfound;
    vector<Vertex::VertexId> nids;
    Vertex::VertexId prev = Vertex::NullVertexId;
    for(int i = 0; i < ids.size(); i++) {
        if(ids[i] != prev) {
            nids.push_back(ids[i]);
            nfound.push_back(found[i]);
            npts.push_back(pts[i]);
            prev = ids[i];
        }
    }

    found = nfound;
    ids = nids;
    pts = npts;
}
/**
 * @brief RoutingPlugin::notifyWay
 * @param way
 */
void RoutingPlugin::notifyWay(OSMWay* way) {
    // filter the edge
    if(!edgeFilter_->acceptEdge(way))
        return;

    // check alone nodes
    if(way->nRefs <= 1)
        return;

    // get the vertices on the way
    vector<VertexId> vers;
    for(int i=0; i < way->nRefs; ++i) {
        // both nodes should be present
        Vertex curr = osmValidator_->findVertex(way->nodeRefs[i]);
        if(curr != Vertex::NullVertexId)
            vers.push_back(curr.getId());
    }

    // get all points at once
    vector<Point> pts;
    vector<int8_t> found;
    vpIndex_.getPoints(vers, pts, found); 
    fixWay(vers, pts, found);   

    // change the order of points if needed
    if(edgeFilter_->reverseEdge(way)) {
        reverse(found.begin(), found.end());
        reverse(vers.begin(), vers.end());
        reverse(pts.begin(), pts.end());
    }

    // set the common type of the edge
    EdgeType commonType = 0;
    if(edgeFilter_->onewayEdge(way))
        commonType |= Edge::ONE_WAY;

    // find the first endpoint with the coordinates
    int prev = 0;
    while(prev < found.size() && !found[prev]) {
        prev++;
    }

    // current, previous, previous endpoint
    DistType dist = 0, time = 0;
    for(int curr = prev+1; curr < found.size(); curr++) {
        // update edge metrics
        if(found[curr-1] && found[curr])
            dist += pointDistance(pts[curr-1], pts[curr]);

        if(isEndPoint(vers[curr])) {
            if(!found[curr] || !found[prev]) {
                LOGG(Logger::ERROR) << "End points are not found: " << prev << " " << curr << " " << way->nID << Logger::FLUSH;
                exit(EXIT_FAILURE);
            }

            // all points in the segment between curr and prev should be found
            if(!(found[curr-1] && found[prev+1])) {
                prev = curr;
                continue;
            }
            for(int i = prev; i <= curr; i++) {
                if(!found[i]) {
                    LOGG(Logger::ERROR) << "Segment doesn't have all points with coordinates: " << way->nID << Logger::FLUSH;
                    exit(EXIT_FAILURE);
                }
            }

            EdgeType type = commonType;

            // serialize only in case not both of the endpoints were turn
            // restricted and there was no point in between curr and prevEnd
            if(!(curr-prev == 1 && isRestricted(vers[curr]) && isRestricted(vers[prev]))) {
                // type tells us if the geometry is stored in the database
                if(curr-prev > 1)
                    type |= Edge::GEOMETRY_STORED;

                // extract geometry
                vector<Point> geom;
                vector<VertexId> ids;
                for(int i = prev+1; i < curr; i++) {
                    geom.push_back(pts[i]);
                    ids.push_back(vers[i]);
                }

                // prepare turn restrictions by inserting a split vertex
                // in the middle of each edge that has at least one restricted
                // endpoint.
                int fix[] = {prev, curr};
                bool changedEndPoint[] = {false, false};
                VertexId newEndIds[] = {newId(vers[prev]), newId(vers[curr])};
                for(int i = 0; i < 2; i++) {
                    // insert a new point just after start of the edge
                    // and before the end of the edge
                    VertexId at = fix[i];
                    if(isRestricted(vers[at])) {
                        vector<Point> tmp = {pts[at-i], pts[at+1-i]};
                        Point mid = centerOfMass(tmp);
                        VertexId newEnd = nextEndPoint();
                        VertexPoint vpt(newEnd, mid.lat(), mid.lon());
                        if(dummyEndPoints_.count({(Edge::EdgeId)way->nID, edgeKey(vers[at-i], vers[at+1-i])})) {
                            LOGG(Logger::ERROR) << "Couldn't resolve way: " << way->nID << Logger::FLUSH;
                            for(int k = 0; k < way->nRefs; k++) {
                                LOGG(Logger::ERROR) << vers[k] << Logger::FLUSH;
                            }
                            exit(EXIT_FAILURE);
                        }
                        dummyEndPoints_[{(Edge::EdgeId)way->nID, edgeKey(vers[at-i], vers[at+1-i])}] = vpt;
                        changedEndPoint[i] = true;
                        newEndIds[i] = newEnd;
                    }
                }

                // update distance
                if(changedEndPoint[0])
                    dist -= pointDistance(pts[prev], pts[prev+1])/2.0;
                if(changedEndPoint[1])
                    dist -= pointDistance(pts[curr], pts[curr-1])/2.0;

                // update edge metrics
                Edge::EdgeDist speed = edgeFilter_->speedLimit(way);
                time += dist/(speed*EdgeFilter::KMH_TO_MS);

                // remember the edges that lead to a turn restriction and from turn restriction
                // schema:
                //    original edge with a turn restriction on both vertices
                //      v1-------v2
                //    new edges created v1->n1, n2->v2
                //      v1--n1------n2-v2
                if(changedEndPoint[0]) {
                    DistType fromDist = pointDistance(pts[prev], pts[prev+1])/2.0;
                    DistType fromTime = fromDist/speed;
                    createTurnEdges(newId(vers[prev]), newEndIds[0], fromDist, fromTime, type, way->nID, true);
                }

                if(changedEndPoint[1]) {
                    DistType toDist = pointDistance(pts[curr], pts[curr-1])/2.0;
                    DistType toTime = toDist/speed;
                    createTurnEdges(newEndIds[1], newId(vers[curr]), toDist, toTime, type, way->nID, false);
                }

                if(edgeFilter_->onewayEdge(way)) {
                    serializeEdge(newEndIds[0], newEndIds[1], dist, time, type, way->nID);
                    serializeGeometry(newEndIds[0], newEndIds[1], true, ids, geom);
                } else {
                    // create and serialize edges
                    serializeEdge(newEndIds[1], newEndIds[0], dist, time, type, way->nID);
                    serializeEdge(newEndIds[0], newEndIds[1], dist, time, type, way->nID);
                    serializeGeometry(newEndIds[0], newEndIds[1], false, ids, geom);
                }
            } else {
                // the case that both vertices are restricted and there
                // is no vertex in between so that the edge could be split

                // create new via vertex
                vector<Point> tmp = {pts[prev], pts[curr]};
                Point mid = centerOfMass(tmp);
                VertexId newEnd = nextEndPoint();
                VertexPoint vpt(newEnd, mid.lat(), mid.lon());
                if(dummyEndPoints_.count({(Edge::EdgeId)way->nID, edgeKey(vers[prev], vers[curr])}) != 0) {
                    LOGG(Logger::ERROR) << "Couldn't resolve way: " << way->nID << Logger::FLUSH;
                    for(int k = 0; k < way->nRefs; k++) {
                        LOGG(Logger::ERROR) << vers[k] << Logger::FLUSH;
                    }
                    exit(EXIT_FAILURE);
                }
                dummyEndPoints_[{(Edge::EdgeId)way->nID, edgeKey(vers[prev], vers[curr])}] = vpt;

                // update edge metrics
                DistType nDist = pointDistance(pts[prev], pts[curr])/2.0;
                DistType speed = edgeFilter_->speedLimit(way);
                DistType nTime = nDist/(speed*EdgeFilter::KMH_TO_MS);

                // create turn edges
                createTurnEdges(newId(vers[prev]), newEnd, nDist, nTime, type, way->nID, true);
                createTurnEdges(newEnd, newId(vers[curr]), nDist, nTime, type, way->nID, false);
            }
            dist = time = 0;
            prev = curr;
        }
    }
}
/**
 * @brief validate called when plugin is done
 */
void RoutingPlugin::validate() {
    // validate endpoints and non-endpoints
    SqlStream sStream;
    URL url(outputFileName_);
    Properties props = {{"type", "sqlite"}, {"create", "0"}};
    sStream.open(url, props);

    size_t rCount = sStream.getNumRows(SqlConsts::KDTREE_ENDPT_TABLE);
    if (rCount != totalEndpointsWritten_)
        die(pluginId_, "Wrong row count in "+SqlConsts::KDTREE_ENDPT_TABLE+
            " "+lexical_cast(rCount)+", expected "+lexical_cast(totalEndpointsWritten_));

    rCount = sStream.getNumRows(SqlConsts::KDTREE_NON_ENDPOINT_TABLE);
    if (rCount != totalNonEndpointsWritten_)
        die(pluginId_, "Wrong row count in "+SqlConsts::KDTREE_NON_ENDPOINT_TABLE+
            " "+lexical_cast(rCount)+", expected "+lexical_cast(totalNonEndpointsWritten_));

    return;
    // validate routing
    int foundPaths = 0, notFound = 0;
    vector<pair<Point, Point> > badRequests;

    Properties prop = {{"type", "sqlite"}, {"create", "0"}, {"table", SqlConsts::KDTREE_ENDPT_TABLE}};

    if(sStream.open(url, prop)) {
        vector<Point> pts;
        Graph<AdjacencyList> graph;
        graph.parseGraph(outputFileName_);
        int hardLimit = 1000;
        while(sStream.getNext() && hardLimit--) {
            CoordType lat, lon;
            VertexId id, start, end;
            sStream >> id >> lat >> lon >> start >> end;
            pts.push_back(Point(lat, lon));
            if(pts.size() >= 5 || !sStream.getNext()) {
                for(int i = 0; i < pts.size(); i++)
                    for(int j = 0; j < pts.size(); j++) {
                        OsmGraphCore::SearchResultBasic result;
                        graph.shortestPathBidirectionalDijkstra(pts[i], pts[j], result);
                        if(result.isValid())
                            foundPaths++;
                        else {
                            badRequests.push_back({pts[i], pts[j]});
                            notFound++;
                        }
                    }
                pts.clear();
            }
        }

        LOGG(Logger::ERROR) << "found: " << foundPaths << " notFound: " << notFound << Logger::FLUSH;
        for(int i = 0; i < badRequests.size(); i++) {
            LOGG(Logger::ERROR) << badRequests[i].first << "->" << badRequests[i].second << Logger::FLUSH;
        }
    } else {
        die(pluginId_, "failed to open vertices table");
    }
}
/**
 * @brief cleanUp
 */
void RoutingPlugin::cleanUp(){
    std::remove(vericesFileName_.c_str());
    std::remove(edgeFileName_.c_str());
    std::remove(endPointsFileName_.c_str());
    std::remove(turnRestrictionsName_.c_str());
    kdTreeEndPt_.cleanUp();
    kdTreeNonEndPt_.cleanUp();
}
/**
 * @brief RoutingPlugin::getTableNamesToImport
 * @return
 */
vector<string> RoutingPlugin::getTableNamesToImport() const {
    vector<string> tables = {SqlConsts::VERTICES_TABLE,
                             SqlConsts::EDGES_TABLE};
    // import endpoints
    vector<string> endPtTables = kdTreeEndPt_.getTableNamesToImport();
    // import non endpoints
    vector<string> nonEndPtTables = kdTreeNonEndPt_.getTableNamesToImport();
    endPtTables.insert(endPtTables.end(), nonEndPtTables.begin(),nonEndPtTables.end());
    tables.insert(tables.end(), endPtTables.begin(), endPtTables.end());
    if(hasTurnRestrictions_)
        tables.push_back(SqlConsts::TURN_RESTRICTIONS);
    return tables;
}
/**
 * @brief getSqlToCreateTables
 * @return
 */
vector<string> RoutingPlugin::getSqlToCreateTables() const {
    vector<string> sql = {SqlConsts::CREATE_KDTREE_ENDPT,
                          SqlConsts::CREATE_KDTREE_NONENDPT,
                          SqlConsts::CREATE_KDTREE_NONENDPT_DATA,
                          SqlConsts::CREATE_VERTICES,
                          SqlConsts::CREATE_EDGES};

    if (hasTurnRestrictions_)
        sql.push_back(SqlConsts::CREATE_TURN_RESTRICTIONS);
    return sql;
}
/**
 * @brief RoutingPlugin::getOtherSqlCommands
 * @return
 */
vector<string> RoutingPlugin::getOtherSqlCommands() const {
    return {SqlQuery::ci("geometry",{"ver1","ver2"})};
}
