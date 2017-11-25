#pragma once

#include <google/dense_hash_map>
#include <google/dense_hash_set>

#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/GraphCore/ModelInterface.h>
#include <UrbanLabs/Sdk/GraphCore/EdgesGTFS.h>
#include <UrbanLabs/Sdk/GraphCore/VerticesGTFS.h>
#include <UrbanLabs/Sdk/GraphCore/SearchResultGtfs.h>
#include <UrbanLabs/Sdk/GraphCore/Graph.h>
#include <UrbanLabs/Sdk/Storage/Storage.h>
#include <UrbanLabs/Sdk/Storage/KdTreeSql.h>
#include <UrbanLabs/Sdk/Utils/Logger.h>

class AdjacencyListGTFS;

class AdjacencyListGTFS : public Model<AdjacencyListGTFS> {
public:
    typedef EdgeGTFS EdgeType;
    typedef VertexGTFS VertexType;
    typedef VertexGTFS::VertexId VertexId;
    typedef VertexId VertexRank;
    typedef Point::CoordType CoordType;
    typedef EdgeGTFS::EdgeDist DistType;
    typedef OsmGraphCore::NearestPointResult NearestPointResult;
    typedef GtfsGraphCore::SearchResultGtfs SearchResult;
public:
    // translate vertex ids to 0 based and contuguous
    typedef vector<Point> VertexPointVector;
private:
    typedef vector<vector<EdgeForw> > VertexEdgeForwVector;
    typedef vector<vector<EdgeBack> > VertexEdgeBackVector;

    // the edge structure of the graph
    typedef vector<vector<Stop::StopId> > StopToStopMap;
    typedef vector<vector<EdgeGTFS> > StopToStopMapDebug;

    typedef vector<vector<pair<Stop::StopId, Trip::TripId> > > StopToStopTripIdMap;
    // for every edge stores which trips occur on this edge
    typedef vector<vector<vector<Trip::TripId> > > EdgeToTrip;
    typedef vector<vector<vector<Trip::TripId> > > EdgeFromTrip;
    // trip storage
    typedef vector<Trip> TripIdToTrip;
    // for a given trip id and a stop find the stop time
    typedef vector<map<Stop::StopId, StopTime> > TripIdStopIdToStopTime;
    // maps a given trip id to a calendar service
    typedef vector<Calendar> ServiceIdToCalendar;

    // iterator types for edges
    typedef typename vector<EdgeGTFS>::iterator OutgoingEdgeIter;
    typedef typename vector<EdgeGTFS>::iterator IncomingEdgeIter;
private:
    // graph edges
    StopToStopMap edgesTo_;
    StopToStopMap edgesFrom_;
    StopToStopMapDebug edgesToDebug_;
    StopToStopMapDebug edgesFromDebug_;
    // maps an edge to list of possible trip ids
    EdgeToTrip edgesToTrip_;
    EdgeFromTrip edgesFromTrip_;
    // a list of trip ids
    TripIdToTrip tripIdToTrip_;
    // maps a service id to a calendar
    ServiceIdToCalendar serviceIdToCalendar_;
    // for a given trip and stop find stop times
    TripIdStopIdToStopTime tripIdStopTimeToStopTime_;
    //--------------------------------------------------------------------------
    // Disk based kd tree for vertex indexing
    KdTreeSql kdTreeDisk_;
    //--------------------------------------------------------------------------
    size_t numVertices_;
    VertexPointVector vertexToPoint_;
public:
    class AlgorithmInit {
    public:
        VertexGTFS s_;
        VertexGTFS d_;
        NearestPointResult srcResult_;
        NearestPointResult dstResult_;
    public:
        bool isSrcEndPoint() const{
            return true;
        }

        bool isDstEndPoint() const {
            return true;
        }

        bool isEndReached(const VertexId &v) const {
            return v == d_.getId();
        }

        VertexId getSrc() const {
            return s_.getId();
        }

        VertexId getDst() const {
            return d_.getId();
        }

        void setDst(VertexId d) {
            d_ = d;
        }

        void setSrc(VertexId s) {
            s_ = s;
        }

        void setSearchResults(const NearestPointResult &src, const NearestPointResult &dst) {
            srcResult_ = src;
            dstResult_ = dst;
        }

        NearestPointResult getSrcSearchResult() const {
            return srcResult_;
        }

        NearestPointResult getDstSearchResult() const {
            return dstResult_;
        }
    };

    class AlgorithmState {
    protected:
        const AdjacencyListGTFS *gModel_;
    public:
        typedef typename google::dense_hash_map<VertexId, size_t, hash<VertexId> > VertexPosMap;
        typedef typename google::dense_hash_map<VertexId, DistType, hash<VertexId> > VertexDistanceMap;
        typedef typename google::dense_hash_map<VertexId, VertexId, hash<VertexId> > VertexVertexMap;
    public:
        // distance map
        typedef VertexDistanceMap DistanceMap;
        // previous vertex map for dijkstra
        typedef VertexVertexMap PrevMap;
        // used for caching heuristic values by A* or similar
        typedef VertexDistanceMap HeuristicMap;
        // was the vertex visited
        typedef vector<bool> VisitedMap;
        // heap type
        typedef Heap<VertexId, DistType, VertexPosMap> HeapType;
    protected:
        HeapType heap_;
        PrevMap prevMap_;
        VisitedMap visMap_;
        DistanceMap oldDist_;
    public:
        AlgorithmState(const AdjacencyListGTFS *model)
            : gModel_(model), visMap_(gModel_->getNumVertices(), 0) {;}

        template<typename T, typename K>
        inline void initMap(T &mapObject) const {
            mapObject.set_empty_key((K)(-1));
            mapObject.set_deleted_key((K)(-2));
        }

        inline void init(const AlgorithmInit &initConfig) {
            initMap<DistanceMap, VertexId>(oldDist_);
            initMap<PrevMap, VertexId>(prevMap_);

            // initialize the heap
            heap_.initGoogle();
            heap_.pushHeap(initConfig.s_.getId(), 0);

            // initialize distance map
            oldDist_[initConfig.s_.getId()] = 0;
            visMap_[initConfig.s_.getId()] = true;
        }

        inline bool isDone() const {
            return heap_.empty();
        }

        inline bool wasSeen(VertexId v) const {
            return visMap_[v];
        }

        inline DistType distTo(VertexId v) const {
            return oldDist_.find(v)->second;
        }

        inline pair<VertexId, DistType> getNextVertex() {
            auto curr = heap_.topHeap();
            heap_.popHeap();
            return curr;
        }

        inline VertexId getStartPoint(const AlgorithmInit &state, VertexId s = VertexGTFS::NullVertexId) const {
            if(s == VertexGTFS::NullVertexId)
                s = state.s_.getId();
            return s;
        }

        inline vector<VertexId> unrollPath(VertexId s, VertexId d, bool rev = false) const {
            return unrollPath(s, d, prevMap_, rev);
        }

        template<typename EdgeIter, typename Metric=EdgeGTFS::DistanceMetric>
        inline void relaxEdge(const pair<VertexId, DistType> &currMin, const EdgeIter &outGoingCur) {
            VertexType currVert = outGoingCur->getNext();

            // find out the new distance
            DistType newDist = currMin.second+outGoingCur->template getCost<Metric>();

            // if the vertex has not been visited yet
            VertexId currVertId = currVert.getId();
            if(!visMap_[currVertId]) {
                visMap_[currVertId] = true;
                prevMap_[currVertId] = currMin.first;
                oldDist_[currVertId] = newDist;

                heap_.pushHeap(currVertId, newDist);
            }
            // if the vertex has been visited before
            else {
                if(oldDist_[currVertId] > newDist) {
                    prevMap_[currVertId] = currMin.first;
                    oldDist_[currVertId] = newDist;
                    heap_.decreaseKey(currVertId, newDist);
                }
            }
        }
    protected:
        /**
         * unrolls path found by dijkstra algorithm
         * @brief unrollPath
         */
        template<class NextVertexMap>
        inline vector<VertexId> unrollPath(VertexId s, VertexId d, NextVertexMap &nextVertex, bool rev = false) const {
            vector<VertexId> path;
            for(VertexId curr = d; curr != s; ) {
                VertexId next = nextVertex.find(curr)->second;
                if(rev)
                    path.push_back(curr);
                else
                    path.push_back(next);
                curr = next;
            }

            path.push_back(s);
            if(!rev) {
                reverse(path.begin(), path.end());
            }

            return path;
        }
    };

    //class AlgorithmStateLocal {};

    //class AlgorithmStateAStar {};

private:
    /**
     * For a given stopId and a given time, iterate over all of the stops
     * connected to the stop.
     * @brief The EdgeIterator class
     */
    class EdgeIterator {
    private:
        size_t seqNum_;
        size_t currEdge_;
        DateTime stopDate_;
        Stop::StopId stopId_;
        AdjacencyListGTFS &graph_;
        // found parameters for an edge
        Trip::TripId bestTripId_;
        StopTime::StopTimeDiff bestDiff_;
    public:
        EdgeIterator(AdjacencyListGTFS &graph, Stop::StopId stopId, DateTime stopTime)
            : seqNum_(0), currEdge_(0), stopDate_(stopTime), stopId_(stopId), graph_(graph)
        {
            while(currEdge_ < graph_.edgesFrom_[stopId_].size() &&
                  !existsEdge(bestTripId_, bestDiff_)) {
                currEdge_++;
            }

            if(currEdge_ == graph_.edgesFrom_[stopId_].size())
                seqNum_ = -1;
        }

        EdgeGTFS operator -> () const {
            Stop::StopId nextId = graph_.edgesFrom_[stopId_][currEdge_];
            return EdgeGTFS(bestTripId_, nextId, bestDiff_);
        }

    private:
        // check if the edgeNum_ is a valid edge
        bool existsEdge(Trip::TripId &bestTripId, StopTime::StopTimeDiff &bestTimeDiff) {
            bool exists = false;
            // iterate over all of the edges for
            for(int i = 0; i < graph_.edgesToTrip_[stopId_][currEdge_].size(); i++) {
                Trip::TripId tripId = graph_.edgesToTrip_[stopId_][currEdge_][i];
                Trip trip = graph_.tripIdToTrip_[tripId];
                Calendar calendar = graph_.serviceIdToCalendar_[trip.getServiceId()];

                DateTime nextDate = calendar.nextDate(stopDate_);
                if(nextDate != DateTime::invalid_) {
                    StopTime stopTime = graph_.tripIdStopTimeToStopTime_[tripId][stopId_];
                    Time depart = stopTime.getDeparture(), arrive = stopTime.getArrival();
                    if(stopDate_.getTime() < depart) {
                        StopTime::StopTimeDiff timeDiff = stopDate_.getTime().getDiff(arrive);
                        if(bestTimeDiff > timeDiff) {
                            bestTimeDiff = timeDiff;
                            bestTripId = tripId;
                        }
                        exists = true;
                    }
                }
            }
            return exists;
        }

    public:
        void operator++() {
            currEdge_++;
            while(currEdge_ < graph_.edgesFrom_[stopId_].size() &&
                  !existsEdge(bestTripId_, bestDiff_)) {
                currEdge_++;
            }

            if(currEdge_ == graph_.edgesFrom_[stopId_].size()) {
                seqNum_ = -1;
                return;
            }
        }
        bool operator == (const EdgeIterator &iterator) const {
            return seqNum_ == iterator.seqNum_;
        }
        bool operator != (const EdgeIterator &iterator) const {
            return seqNum_ != iterator.seqNum_;
        }
    };
public:

    bool getInitConfig(const Point &src, const Point &dst, AlgorithmInit &initConfig) {
        // find out the overall start of the route
        NearestPointResult nearestSrc, nearestDst;
        if(!findNearestPointKdTree(src, nearestSrc)) {
            return false;
        }
        if(!findNearestPointKdTree(dst, nearestDst)) {
            return false;
        }

        VertexId endVertex = nearestDst.getTarget().getId();
        VertexId startVertex = nearestSrc.getTarget().getId();

        // set source and destination
        initConfig.setDst(endVertex);
        initConfig.setSrc(startVertex);
        initConfig.setSearchResults(nearestSrc, nearestDst);
        return true;
    }

    /**
     * @brief initKdTreeSpatial
     * @param indexFile
     * @param stops
     */
    void initKdTreeSpatial(const string& indexFile, vector<Stop>& stops) {
        string bulkFileName = indexFile + ".bulk";
        ifstream stopsBulk(bulkFileName);

        string sep = StringConsts::SEPARATOR, noStartEnd = sep;
        if(!stopsBulk.good()) {
            kdTreeDisk_.openForBulkLoad(indexFile, bulkFileName);
            for (Stop &stop : stops) {
                Vertex ver(stop.getId());
                VertexPoint vp(ver, stop.getPoint());
                kdTreeDisk_.insertBulk(vp, sep, noStartEnd);
            }
            //kdTreeDisk_.bulkLoadTreeFileStream<BulkFileDataStream>();
        }
//        SqlStream str;
//        kdTreeDisk_.init(indexFile, &str);
    }

    /**
     * TODO: check sqlite database initialization
     * @brief parse
     * @param filename
     */
    bool parse(const string &/*filename*/) {
        string folder = "example/example/";

        // do not change the order
        vector<Agency> agencies = readObjects<Agency>(folder+"agency.txt");

        vector<Stop> stops = readObjects<Stop>(folder+"stops.txt");
        // initialize vertexToPoint_
        vertexToPoint_.resize(Stop::translator_.totalIds());
        for(int i = 0; i < vertexToPoint_.size(); i++) {
            vertexToPoint_[stops[i].getId()] = stops[i].getPoint();
        }

        vector<Route> routes = readObjects<Route>(folder+"routes.txt");

        serviceIdToCalendar_ = readObjects<Calendar>(folder+"calendar.txt");

        tripIdToTrip_ = readObjects<Trip>(folder+"trips.txt");

        // sort by trip id, then by sequence numbers
        vector<StopTime> stopTimes = readObjects<StopTime>(folder+"stop_times.txt");
        sort(stopTimes.begin(), stopTimes.end());

        // initialize edges
        initEdges(stopTimes);

        // initialize lookup of tripId, stopId -> stopTime
        initTripIdStopIdToStopTime(stopTimes);

        // initialize nearest neigbor lookup
        initKdTreeSpatial(folder+"stops", stops);

        numVertices_ = stops.size();
        return true;
    }

    /**
     * Releases memory held by the object
     */
    template<typename T>
    void releaseMemory(T &t) {
        T dummy;
        swap(t, dummy);
    }

    /**
     * @brief unload
     */
    void unload() {
        releaseMemory(edgesTo_);
        releaseMemory(edgesFrom_);
        releaseMemory(edgesToDebug_);
        releaseMemory(edgesFromDebug_);
        releaseMemory(edgesToTrip_);
        releaseMemory(edgesFromTrip_);
        releaseMemory(tripIdToTrip_);
        releaseMemory(serviceIdToCalendar_);
        releaseMemory(tripIdStopTimeToStopTime_);
        kdTreeDisk_.close();
        releaseMemory(vertexToPoint_);
    }

    /**
     * @brief initTripIdStopIdToStopTime
     * @param stopTimes
     */
    void initTripIdStopIdToStopTime(const vector<StopTime> &stopTimes) {
        tripIdStopTimeToStopTime_.resize(Trip::translator_.totalIds());
        Trip::TripId currId = Trip::NullTripId;

        // collect stops together with their trip ids
        for(const StopTime &stopTime : stopTimes) {
            if(stopTime.getTripId() != currId) {
                currId = stopTime.getTripId();
            }
            tripIdStopTimeToStopTime_[currId][stopTime.getStopId()] = stopTime;
        }
    }

    /**
     * @brief initEdges
     * @param stopTimes
     */
    void initEdges(const vector<StopTime> &stopTimes) {
        if(stopTimes.size() == 0) {
            LOGG(Logger::WARNING) << "stop times empty" << Logger::FLUSH;
            return;
        }

        // prepare private fields
        edgesTo_.resize(Stop::translator_.totalIds());
        edgesFrom_.resize(Stop::translator_.totalIds());

        edgesToDebug_.resize(Stop::translator_.totalIds());
        edgesFromDebug_.resize(Stop::translator_.totalIds());

        edgesToTrip_.resize(Stop::translator_.totalIds());
        edgesFromTrip_.resize(Stop::translator_.totalIds());

        StopToStopTripIdMap edgesToTmp(Stop::translator_.totalIds());
        StopToStopTripIdMap edgesFromTmp(Stop::translator_.totalIds());

        // create edges bases on stop ids and trip ids
        collectStopIdsAndTripIds(stopTimes, edgesToTmp, edgesFromTmp, edgesToDebug_, edgesFromDebug_);

        // extract trip ids to a separate structure
        extractEdgesAndTripIds(edgesToTmp, edgesToTrip_, edgesTo_);
        extractEdgesAndTripIds(edgesFromTmp, edgesFromTrip_, edgesFrom_);
    }

    /**
     * @brief collectStopIdsAndTripIds
     * @param stopTimes
     * @param edgesToTmp
     * @param edgesFromTmp
     * @param edgesToDebug
     * @param edgesFromDebug
     */
    void collectStopIdsAndTripIds(const vector<StopTime> &stopTimes,
                                  StopToStopTripIdMap &edgesToTmp,
                                  StopToStopTripIdMap &edgesFromTmp,
                                  StopToStopMapDebug &edgesToDebug,
                                  StopToStopMapDebug &edgesFromDebug) {

        vector<Stop::StopId> stops;
        vector<Time> timeArrive, timeDepart;
        Trip::TripId currId = Trip::NullTripId;

        // collect stops together with their trip ids
        for(int i = 0; i < stopTimes.size(); i++) {
            const StopTime stopTime = stopTimes[i];
            if(stopTime.getTripId() != currId) {
                for(int j = 0; j < stops.size(); j++) {
                    if(j > 0) {
                        edgesToTmp[stops[j]].push_back({stops[j-1], currId});
                        EdgeGTFS edge = {currId, stops[j-1], timeArrive[j].getDiff(timeDepart[j-1])};
                        edge.setTime(edge.getCost<EdgeGTFS::DistanceMetric>());
                        edgesToDebug[stops[j]].push_back(edge);
                    }
                    if(i+1 < stops.size()) {
                        edgesFromTmp[stops[j]].push_back({stops[j+1], currId});
                        EdgeGTFS edge = {currId, stops[j+1], timeArrive[j+1].getDiff(timeDepart[j])};
                        edge.setTime(edge.getCost<EdgeGTFS::DistanceMetric>());
                        edgesFromDebug[stops[j]].push_back(edge);
                    }
                }
                currId = stopTime.getTripId();
                stops = {stopTime.getStopId()};
                timeArrive = {stopTime.getArrival()};
                timeDepart = {stopTime.getDeparture()};
            } else {
                stops.push_back(stopTime.getStopId());
                timeArrive.push_back(stopTime.getArrival());
                timeDepart.push_back(stopTime.getDeparture());
            }
        }

        if(stops.size() > 1) {
            for(int i = 0; i < stops.size(); i++) {
                if(i > 0) {
                    edgesToTmp[stops[i]].push_back({stops[i-1], currId});
                    EdgeGTFS edge = {currId, stops[i-1], timeArrive[i].getDiff(timeDepart[i-1])};
                    edge.setTime(edge.getCost<EdgeGTFS::DistanceMetric>());
                    edgesToDebug[stops[i]].push_back(edge);
                }
                if(i+1 < stops.size()) {
                    edgesFromTmp[stops[i]].push_back({stops[i+1], currId});
                    EdgeGTFS edge = {currId, stops[i+1] , timeArrive[i+1].getDiff(timeDepart[i])};
                    edge.setTime(edge.getCost<EdgeGTFS::DistanceMetric>());
                    edgesFromDebug[stops[i]].push_back(edge);
                }
            }
        }
    }

    /**
     *
     */
    template<typename T, typename TT, typename TTT>
    void extractEdgesAndTripIds(T &edgesToTmp, TT &edgesToTrip, TTT &edgesTo) {
        for(int v = 0; v < Stop::translator_.totalIds(); v++) {
            sort(edgesToTmp[v].begin(), edgesToTmp[v].end());

            vector<Trip::TripId> trips;
            Stop::StopId currStopId = Stop::NullStopId;
            for(int i = 0; i < edgesToTmp[v].size(); i++) {
                if(edgesToTmp[v][i].first != currStopId) {
                    // extract trip ids for each edge
                    if(trips.size() > 0)
                        edgesToTrip[v].push_back(trips);
                    currStopId = edgesToTmp[v][i].first;
                    trips = {edgesToTmp[v][i].second};

                    // create the actual edge
                    edgesTo[v].push_back(currStopId);
                } else {
                    trips.push_back(edgesToTmp[v][i].second);
                }
            }

            if(trips.size() > 0)
                edgesToTrip[v].push_back(trips);
        }
    }

    /**
     * find nearest point using kd-tree datastructure
     * @brief findNearestPointKdTree
     * @param pt
     * @return
     */
     bool findNearestPointKdTree(const Point &pt, NearestPointResult &result) {
        //TODO lock the Tree!
        string data;
        if(!kdTreeDisk_.findNearestVertex(pt, result, data))
            return false;
        return true;
     }

    /**
    * @brief findOrigWayIds
    * @return
    */
    bool findOrigWayIds(const vector<VertexGTFS::VertexId> &/*path*/, vector<EdgeGTFS::EdgeId> &/*wayIds*/) {
        return false;
    }

    /**
     * @brief findOrigWayId
     * @param s
     * @param d
     * @param id
     * @return
     */
    bool findOrigWayId(VertexGTFS::VertexId /*s*/, VertexGTFS::VertexId /*d*/, EdgeGTFS::EdgeId &/*id*/) {
        return false;
    }

    /**
     * preprocess if needed, pass a reference to a graph of this model
     */
    void preprocess() {
        ;
    }

    /**
     * get an intermediate vertex
     */
    VertexId getViaForEdge(VertexType /*from*/, VertexType /*to*/) const {
        return Vertex::NullVertexId;
    }

    /**
     * find geometry for edge
     */
    void findGeometryForEdge(const VertexId source, const VertexId dest,
                             const Point &/*target*/, vector<Point> &geometry) const {
        geometry = {getPoint(VertexType(source)), getPoint(VertexType(dest))};
    }

    /**
     * get point
     */
    Point getPoint(const VertexType &v) const {
        return vertexToPoint_[v.getId()];
    }

    /**
     * @brief getNumVertices
     * @return
     */
    size_t getNumVertices() const {
        return numVertices_;
    }

    /**
     * get outgoing edge iterators
     * @brief getOutgoingIterBegin
     * @param v
     * @return
     */
    OutgoingEdgeIter getOutgoingIterBegin(const VertexType &v) {
        return edgesFromDebug_[v.getId()].begin();
    }

    /**
     * @brief getOutgoingIterEnd
     * @param v
     * @return
     */
    OutgoingEdgeIter getOutgoingIterEnd(const VertexType &v) {
        return edgesFromDebug_[v.getId()].end();
    }

    /**
     * get incoming edges
     * @brief getIncomingIterBegin
     * @param v
     * @return
     */
    IncomingEdgeIter getIncomingIterBegin(const VertexType &v) {
        abort();
        return edgesToDebug_[v.getId()].begin();
    }

    /**
     * @brief getIncomingIterEnd
     * @param v
     * @return
     */
    IncomingEdgeIter getIncomingIterEnd(const VertexType &v) {
        abort();
        return edgesToDebug_[v.getId()].end();
    }
};
