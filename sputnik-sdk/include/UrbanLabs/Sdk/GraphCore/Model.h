#pragma once

#include <mutex>
#include <stack>
#include <thread>
#include <google/dense_hash_map>
#include <google/dense_hash_set>

#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/GraphCore/Edges.h>
#include <UrbanLabs/Sdk/GraphCore/Vertices.h>
#include <UrbanLabs/Sdk/GraphCore/TurnRestriction.h>
#include <UrbanLabs/Sdk/GraphCore/ModelInterface.h>
#include <UrbanLabs/Sdk/GraphCore/SearchResult.h>
#include <UrbanLabs/Sdk/GraphCore/Graph.h>
#include <UrbanLabs/Sdk/Storage/Storage.h>
#include <UrbanLabs/Sdk/Storage/KdTreeSql.h>
#include <UrbanLabs/Sdk/Storage/SqlConsts.h>
#include <UrbanLabs/Sdk/Utils/Logger.h>

//#define DEBUG_MODEL
class AdjacencyList;

// adjacency matrix based edge model
class AdjacencyList : public Model<AdjacencyList> {
public:
    typedef Edge EdgeType;
    typedef Vertex VertexType;
    typedef Vertex::VertexId VertexId;
    typedef VertexId VertexRank;
    typedef VertexPoint::CoordType CoordType;
    typedef Edge::EdgeDist DistType;
    typedef OsmGraphCore::NearestPointResult NearestPointResult;
    typedef OsmGraphCore::SearchResultBasic SearchResult;
    typedef OsmGraphCore::SearchResultLocal SearchResultLocal;
    // iterator types for edges
    typedef typename std::vector<EdgeForw>::iterator OutgoingEdgeIter;
    typedef typename std::vector<EdgeBack>::iterator IncomingEdgeIter;
public:
    typedef std::vector<Point> VertexPointVector;
private:
    typedef long long HeuristicVal;
private:
    typedef std::vector<Vertex> VertexVector;
    typedef std::vector<std::vector<EdgeForw> > VertexEdgeForwVector;
    typedef std::vector<std::vector<EdgeBack> > VertexEdgeBackVector;
    typedef std::vector<VertexId> VertexRankMap;
    typedef unordered_set<VertexId> UnrankedVertexSet;
    typedef std::vector<unordered_map<VertexId, DistType> > WitnessMap;
    typedef std::vector<TurnRestriction> TurnRestrictions;
public:
    class AlgorithmInit {
    private:
        NearestPointResult srcResult_;
        NearestPointResult dstResult_;
        // types of edges
        Edge::EdgeType srcEdgeType_;
        Edge::EdgeType dstEdgeType_;
    public:
        AlgorithmInit() : srcResult_(), dstResult_(), srcEdgeType_(0), dstEdgeType_(0) {;}

        /**
         * @brief isEndReached
         * @param v
         * @return
         */
        bool isEndReached(VertexId v) const {
            if(!isDstEndPoint()) {
                Edge::EdgeType type = getDstEdgeType();
                if(type & Edge::ONE_WAY) {
                    return v == getDstEdgeSrc();
                } else
                    return v == getDstEdgeSrc() || v == getDstEdgeDst();
            } else
                return v == getDstEdgeSrc();
            return false;
        }

        /**
         * @brief isSrcEndPoint
         * @return
         */
        bool isSrcEndPoint() const {
            return srcResult_.isEndPoint();
        }

        /**
         * @brief isDstEndPoint
         * @return
         */
        bool isDstEndPoint() const {
            return dstResult_.isEndPoint();
        }

        /**
         * @brief setSrcEdgeType
         * @param type
         */
        void setSrcEdgeType(Edge::EdgeType type) {
            srcEdgeType_ = type;
        }

        /**
         * @brief getSrcEdgeType
         */
        Edge::EdgeType getSrcEdgeType() const {
            return srcEdgeType_;
        }

        /**
         * @brief setDstEdgeType
         * @param type
         */
        void setDstEdgeType(Edge::EdgeType type) {
            dstEdgeType_ = type;
        }

        /**
         * @brief getDstEdgeType
         */
        Edge::EdgeType getDstEdgeType() const {
            return dstEdgeType_;
        }

        /**
         * returns reversed init config
         */
        AlgorithmInit reverseConfig() {
            AlgorithmInit revConf;
            revConf.srcEdgeType_ = dstEdgeType_;
            revConf.dstEdgeType_ = srcEdgeType_;
            revConf.setSearchResults(dstResult_.reverse(), srcResult_.reverse());
            return revConf;
        }

        /**
         * @brief getSrcEdgeSrc
         * @return
         */
        VertexId getSrcEdgeSrc() const {
            return srcResult_.getSrc().getId();
        }

        /**
         * @brief getSrcEdgeDst
         * @return
         */
        VertexId getSrcEdgeDst() const {
            return srcResult_.getDst().getId();
        }

        /**
         * @brief getDstEdgeSrc
         * @return
         */
        VertexId getDstEdgeSrc() const {
            return dstResult_.getSrc().getId();
        }

        /**
         * @brief getDstEdgeSrc
         * @return
         */
        VertexId getDstEdgeDst() const {
            return dstResult_.getDst().getId();
        }

        /**
         * @brief getSrcSearchResult
         * @return
         */
        NearestPointResult getSrcSearchResult() const {
            return srcResult_;
        }

        /**
         * @brief getDstSearchResult
         * @return
         */
        NearestPointResult getDstSearchResult() const {
            return dstResult_;
        }

        void setSearchResults(const NearestPointResult &src, const NearestPointResult &dst) {
            srcResult_ = src, dstResult_ = dst;
        }
    };

    class AlgorithmInitLocal {
    public:
        // used by local dijkstra searches during preprocessing
        Vertex src_, dst_;
        size_t hopNumber_;
        size_t searchSpace_;
        DistType stopDistance_;
        VertexId ignoredVertex_;
        unordered_set<VertexId> targetVertices_;
    public:
        /**
         * @brief AlgorithmInitLocal
         */
        AlgorithmInitLocal() : hopNumber_(0), searchSpace_(0), stopDistance_(0),
            ignoredVertex_(VertexType::NullVertexId) {;}

        /**
         * @brief getSrc
         * @return
         */
        Vertex getSrc() const {
            return src_;
        }

        /**
         * @brief getDst
         * @return
         */
        Vertex getDst() const {
            return dst_;
        }

        /**
         * @brief setSrc
         * @param vertex
         */
        void setSrc(const Vertex &vertex) {
            src_ = vertex;
        }

        /**
         * @brief setDst
         * @param vertex
         */
        void setDst(const Vertex &vertex) {
            dst_ = vertex;
        }

        /**
         * stop once this distance is reached
         */
        void setStoppingDistance(DistType stopDist) {
            stopDistance_ = stopDist;
        }

        /**
         * do not visit the ignored vertex
         */
        void setIgnoredVertex(VertexId ignoredVertex) {
            ignoredVertex_ = ignoredVertex;
        }

        /**
         * @brief setSearchSpace
         */
        void setSearchSpace(size_t searchSpace) {
            searchSpace_ = searchSpace;
        }

        /**
         * @brief setHopNumber
         * @param hops
         */
        void setHopNumber(size_t hops) {
            hopNumber_ = hops;
        }

        /**
         * @brief setTargetvertices
         * @param targetVertices
         */
        void setTargetVertices(const unordered_set<VertexId> &targetVertices) {
            targetVertices_ = targetVertices;
        }
    };

    class AlgorithmState {
    protected:
        AdjacencyList *gModel_;
    public:
        typedef typename google::dense_hash_map<VertexId, size_t, hash<VertexId> > VertexPosMap;
        typedef typename google::dense_hash_map<VertexId, DistType, hash<VertexId> > VertexDistanceMap;
        typedef typename google::dense_hash_map<VertexId, VertexId, hash<VertexId> > VertexVertexMap;
        typedef typename google::dense_hash_set<VertexId, hash<VertexId> > VisitedSet;
    public:
        // distance map
        typedef VertexDistanceMap DistanceMap;
        // previous vertex map for dijkstra
        typedef VertexVertexMap PrevMap;
        // used for caching heuristic values by A* or similar
        typedef VertexDistanceMap HeuristicMap;
        // heap type
        typedef Heap<VertexId, DistType, VertexPosMap> HeapType;
    protected:
        bool rev_;
        HeapType heap_;
        PrevMap prevMap_;
        VisitedSet visSet_;
        DistanceMap oldDist_;
    public:
        AlgorithmState(AdjacencyList *model, bool rev = false) : gModel_(model), rev_(rev) {;}

        template<typename T, typename K>
        inline void initMap(T &map) {
            map.set_empty_key((K)(-1));
            map.set_deleted_key((K)(-2));
        }

        inline void init(const AlgorithmInit &initConfig) {
            initMap<VisitedSet, VertexId>(visSet_);
            initMap<DistanceMap, VertexId>(oldDist_);
            initMap<PrevMap, VertexId>(prevMap_);

            Edge::EdgeType type = initConfig.getSrcEdgeType();

            // initialize the heap
            heap_.initGoogle();
            if(initConfig.isSrcEndPoint()) {
                heap_.pushHeap(initConfig.getSrcEdgeSrc(), 0);
                oldDist_[initConfig.getSrcEdgeSrc()] = 0;
                visSet_.insert(initConfig.getSrcEdgeSrc());
            } else {
                // src1 is the source of the edge and src2 is the target
                if((type & Edge::ONE_WAY) == 0) {
                    heap_.pushHeap(initConfig.getSrcEdgeSrc(), 1);
                    heap_.pushHeap(initConfig.getSrcEdgeDst(), 1);

                    oldDist_[initConfig.getSrcEdgeSrc()] = 1;
                    oldDist_[initConfig.getSrcEdgeDst()] = 1;

                    visSet_.insert(initConfig.getSrcEdgeSrc());
                    visSet_.insert(initConfig.getSrcEdgeDst());
                } else {
                    // in case of one way edge we can only traverse forward
                    // starting from the destination of the edge
                    heap_.pushHeap(initConfig.getSrcEdgeDst(), 0);
                    oldDist_[initConfig.getSrcEdgeDst()] = 0;
                    visSet_.insert(initConfig.getSrcEdgeDst());
                }
            }
        }

        inline bool isDone() {
            return heap_.empty();
        }

        inline bool wasSeen(VertexId v) const {
            return visSet_.find(v) != visSet_.end();
        }

        inline DistType distTo(VertexId v) const {
            return oldDist_.find(v)->second;
        }

        inline pair<VertexId, DistType> getNextVertex() {
            auto curr = heap_.topHeap();
            heap_.popHeap();
            return curr;
        }

        inline VertexId getStartPoint(const AlgorithmInit &state, VertexId d = Vertex::NullVertexId) const {
            // the destination is set as not null in case we have launched
            // a bidirectional search. In that case we know the know the destination
            // point (the common meeting vertex)
            if(!state.isSrcEndPoint()) {
                if(!state.isDstEndPoint() && d == Vertex::NullVertexId) {
                    Edge::EdgeType type = state.getDstEdgeType();
                    if(type & Edge::ONE_WAY)
                        d = state.getDstEdgeSrc();
                    else
                        d = prevMap_.find(state.getDstEdgeSrc()) != prevMap_.end() ? state.getDstEdgeSrc() : state.getDstEdgeDst();
                    return unrollPrevMap(d, state, prevMap_);
                } else {
                    if(d == Vertex::NullVertexId) {
                        d = state.getDstEdgeSrc();
                        assert(state.getDstEdgeSrc() == state.getDstEdgeDst());
                    }
                    return unrollPrevMap(d, state, prevMap_);
                }
            } else
                return state.getSrcEdgeSrc();
        }

        inline std::vector<VertexId> unrollPath(VertexId s, VertexId d, bool rev = false) {
            return unrollPath(s, d, prevMap_, rev);
        }

        template<typename EdgeIter, typename Metric=Edge::DistanceMetric>
        inline void relaxEdge(const pair<VertexId, DistType> &currMin, const EdgeIter &outGoingCur) {
            VertexType currVert = outGoingCur->getNext();

            // find out the new distance
            DistType newDist = currMin.second+outGoingCur->template getCost<Metric>();

            // if the vertex has not been visited yet
            VertexId currVertId = currVert.getId();
            if(!wasSeen(currVertId)) {
                visSet_.insert(currVertId);
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
         * @brief getStartingPoint
         * @param dest
         * @param prevMap
         * @return
         */
        inline VertexId unrollPrevMap(const VertexId dest, const AlgorithmInit &state, const PrevMap &prevMap) const {
            VertexId curr = dest, src1 = state.getSrcEdgeSrc(), src2 = state.getSrcEdgeDst();
            Edge::EdgeType type = state.getSrcEdgeType();
            bool isOneWay = type & Edge::ONE_WAY;

            if(isOneWay) {
                // if the source edge is one way we can only go through the destination of the edge
                while(curr != src2) {
                    curr = prevMap.find(curr)->second;
                }
            } else {
                while(curr != src1 && curr != src2) {
                    curr = prevMap.find(curr)->second;
                }
            }

            return curr;
        }

        /**
         * unrolls path found by dijkstra algorithm
         * @brief unrollPath
         */
        template<class NextVertexMap>
        inline std::vector<VertexId> unrollPath(VertexId s, VertexId d, NextVertexMap &nextVertex, bool rev = false) {
            std::vector<VertexId> path;
            for(VertexId curr = d; curr != s; ) {
                std::stack<pair<VertexId, VertexId> > unrollEdges;
                if(rev) {
                    unrollEdges.push({curr, nextVertex[curr]});
                } else {
                    unrollEdges.push({nextVertex[curr], curr});
                }

                while(!unrollEdges.empty()) {
                    pair<VertexId, VertexId> currEdg = unrollEdges.top();
                    unrollEdges.pop();

                    // determine edge type and the via vertex
                    Edge::EdgeType type = 0;
                    VertexId via = Vertex::NullVertexId;
                    {
                        EdgeForw *edgeForw = gModel_->findEdgeForw(currEdg.first, currEdg.second);
                        if(edgeForw == 0) {
                            EdgeBack *edgeBack = gModel_->findEdgeBack(currEdg.first, currEdg.second);
                            type = edgeBack->getType();
                            via = edgeBack->getVia();
                        } else {
                            type = edgeForw->getType();
                            via = edgeForw->getVia();
                        }
                    }

                    // final edges might be created by turn restrictions
                    // we should not expand them further
                    if(via == VertexType::NullVertexId || (type & Edge::FINAL)) {
                        if(rev)
                            path.push_back(currEdg.first);
                        else
                            path.push_back(currEdg.second);
                    }
                    else {
                        if(rev) {
                            unrollEdges.push({via, currEdg.second});
                            unrollEdges.push({currEdg.first, via});
                        } else {
                            unrollEdges.push({currEdg.first, via});
                            unrollEdges.push({via, currEdg.second});
                        }
                    }
                }

                curr = nextVertex[curr];
            }

            path.push_back(s);
            if(!rev) {
                reverse(path.begin(), path.end());
            }

            return path;
        }
    };

    class AlgorithmStateLocal : public AlgorithmState {
    public:
        typedef google::dense_hash_map<Vertex::VertexId, size_t, hash<Vertex::VertexId> > HeapKeyPosMap;
        typedef google::dense_hash_map<VertexId, size_t, hash<VertexId> > HopsMap;
    private:
        HopsMap hopsMap_;
    public:
        AlgorithmStateLocal(AdjacencyList *model) : AlgorithmState(model) {;}

        void init(const AlgorithmInitLocal &state) {
            Vertex s = state.getSrc();
            initMap<HopsMap, VertexId>(hopsMap_);
            hopsMap_[s.getId()] = 0;
            // initialize the heap
            heap_.initGoogle();
            heap_.pushHeap(s.getId(), 0);

            // initialize distance
            initMap<DistanceMap, VertexId>(oldDist_);
            oldDist_[s.getId()] = 0;

            // initializ visited map
            initMap<VisitedSet, VertexId>(visSet_);
            visSet_.insert(s.getId());
        }

        size_t numHops(VertexId v) {
            return hopsMap_[v];
        }

        inline DistType distTo(VertexId id) const {
            auto it = oldDist_.find(id);
            if(it != oldDist_.end())
                return it->second;
            return numeric_limits<DistType>::max();
        }

        template<typename EdgeIter, typename Metric=Edge::DistanceMetric>
        bool relaxEdge(const pair<VertexId, DistType> &currMin, size_t prevHops, const EdgeIter &outGoingCur) {
            VertexType currVert = outGoingCur->getNext();
            if(gModel_->isRanked(currVert.getId()))
                return false;

            // find out the new distance
            DistType newDist = currMin.second+outGoingCur->template getCost<Metric>();

            // if the vertex has not been visited yet
            VertexId currVertId = currVert.getId();
            if(!wasSeen(currVertId)) {
                visSet_.insert(currVertId);
                oldDist_[currVertId] = newDist;

                heap_.pushHeap(currVertId, newDist);
                hopsMap_[currVert.getId()] = prevHops+1;
            }
            // if the vertex has been visited before
            else {
                if(oldDist_[currVertId] > newDist) {
                    oldDist_[currVertId] = newDist;
                    heap_.decreaseKey(currVertId, newDist);
                    hopsMap_[currVert.getId()] = min(hopsMap_[currVert.getId()], prevHops+1);
                }
            }
            return true;
        }
    };

    class AlgorithmStateAStar : public AlgorithmState {
    private:
        DistanceMap distToDest_;
    public:
        AlgorithmStateAStar(AdjacencyList *model) : AlgorithmState(model) {;}

        void init(const AlgorithmInit &state) {
            Vertex s = state.getSrcEdgeSrc(), t = state.getDstEdgeSrc();
            initMap<DistanceMap, VertexId>(distToDest_);
            // points
            Point srcPoint = gModel_->getPoint(s);
            Point destPoint = gModel_->getPoint(t);

            // keep precomputed distance to the destination
            if(state.isSrcEndPoint()) {
                distToDest_[s.getId()] = pointDistance(srcPoint, destPoint);
            } else {
                Point srcPt1 = gModel_->getPoint(state.getSrcEdgeSrc());
                Point srcPt2 = gModel_->getPoint(state.getSrcEdgeDst());
                distToDest_[state.getSrcEdgeSrc()] = pointDistance(srcPt1, destPoint);
                distToDest_[state.getSrcEdgeDst()] = pointDistance(srcPt2, destPoint);
            }
            static_cast<AlgorithmState*>(this)->init(state);
        }

        DistType getHeuristic(VertexId v) {
            return distToDest_[v];
        }

        template<typename EdgeIter, typename Metric=Edge::DistanceMetric>
        void relaxEdge(const pair<VertexId, DistType> &currMin,
                       const Point &destPoint, const DistType heuristicOld,
                       const EdgeIter &outGoingCur) {
            VertexType currVert = outGoingCur->getNext();

            // find out the new distance
            DistType newDist = currMin.second;

            // if the vertex has not been visited yet
            VertexId currVertId = currVert.getId();
            if(!wasSeen(currVertId)) {
                visSet_.insert(currVertId);

                // compute the distance to destination and save it
                Point currPoint = gModel_->getPoint(currVert);
                DistType heuristicNew = pointDistance(currPoint, destPoint);
                distToDest_[currVertId] = heuristicNew;

                // reweight the curent edge
                newDist += max(heuristicNew-heuristicOld+outGoingCur->template getCost<Metric>(), 0);

                prevMap_[currVertId] = currMin.first;
                oldDist_[currVertId] = newDist;
                heap_.pushHeap(currVertId, newDist);
            }
            // if the vertex has been visited before
            else {
                newDist += max(distToDest_[currVertId]-heuristicOld+outGoingCur->template getCost<Metric>(), 0);

                if(oldDist_[currVertId] > newDist) {
                    prevMap_[currVertId] = currMin.first;
                    oldDist_[currVertId] = newDist;
                    heap_.decreaseKey(currVertId, newDist);
                }
            }
        }
    };

private:

    /**
     * compares ranks of two vertices
     */
    class CompareRanks {
    public:
        bool operator()(const pair<Vertex, VertexRank> &pr1,
                        const pair<Vertex, VertexRank> &pr2) {
            return pr1.second < pr2.second;
        }
    };

    /**
     * needed for finding vertex in a vector representation of vertices
     */
    class CompareVertices {
    public:
        Vertex vert_;

        bool operator()(const Edge &edg) {
            return edg.getNextId() == vert_.getId();
        }
    };

    /**
     *
     */
    template<class E>
    class CompareEdge {
    public:
        bool operator () (const E &e1, const E &e2) {
            return e1.getNextId() < e2.getNextId();
        }
    };

    /**
     *
     */
    template<class E>
    class CompareEdgeUnranked {
    public:
        const AdjacencyList* gModel_;
        CompareEdgeUnranked(const AdjacencyList *model) : gModel_(model) {;}
    public:
        bool operator () (const E &e1, const E &e2) {
            if(gModel_->isRanked(e1.getNextId()) != gModel_->isRanked(e2.getNextId())) {
                if(gModel_->isRanked(e1.getNextId()))
                    return false;
                else
                    return true;
            }
            return e1.getNextId() < e2.getNextId();
        }
    };

    /**
     *
     */
    template<class E>
    class CompareEdgeEqual {
    public:
        bool operator () (const E &e1, const E &e2) {
            return e1.getNextId() == e2.getNextId();
        }
    };

    /**
     *
     */
    template<class E>
    class CompareVertexEdge {
    public:
        bool operator () (const E &e1, const Vertex &v) {
            return e1.getNextId() < v.getId();
        }
    };

    /**
     *
     */
    template<class E>
    class CompareVertexEdgeUnranked {
    public:
        const AdjacencyList *gModel_;
        CompareVertexEdgeUnranked(const AdjacencyList *model) : gModel_(model) {;}
    public:
        bool operator () (const E &e1, const Vertex &v) {
            if(gModel_->isRanked(e1.getNextId()))
                return false;
            return e1.getNextId() < v.getId();
        }
    };

    /**
     * @brief The VertexProcResult class
     */
    class VertexProcResult {
    public:
        size_t totalSearchSpace_;
        size_t totalShortcuts_;

        VertexProcResult() : totalSearchSpace_(0), totalShortcuts_(0) {
            ;
        }
    };

private:
    //--------------------------------------------------------------------------
    // number of vertices
    size_t numVertices_;
    //--------------------------------------------------------------------------
    // backward edges
    VertexEdgeBackVector edgesTo_;
    // forward edges
    VertexEdgeForwVector edgesFrom_;
    //--------------------------------------------------------------------------
    // turn restrictions
    TurnRestrictions restrictions_;
    //--------------------------------------------------------------------------
    // number of marked verticed
    size_t numMarkedVert_;
    // rank of the vertex
    VertexRankMap rank_;
    // bool vector marking that the vertex was marked
    std::vector<bool> ranked_;
    // set of unranked nodes
    UnrankedVertexSet unrankedNodes_;
    //--------------------------------------------------------------------------
    // sql based kd tree for endpoint indexing
    KdTreeSql kdTreeEndPt_;
    // sql based kdtree for non endpoints
    KdTreeSql kdTreeNonEndPt_;
    //geometry index in sqlite
    GeometryIndexSql indexGeometry_;
    // vertex to point vector
    VertexPointVector vertexToPoint_;
    //--------------------------------------------------------------------------
    // the input filename
    std::string inputFilename_;
private:
    //--------------------------------------------------------------------------
    // a flag set if we read from
    // already preprocessed input
    bool readPreprocessed_;
    // edges added during preprocessing
    size_t edgesAdded_;
    // static field hop number
    size_t hopsAllowed_;
    // the total degree for vertices
    size_t totalDegree_;
    // the search space limit
    size_t searchSpaceLimit_;
    // the set of witness found distances
    WitnessMap oneHopWitness_;
    // lock
    std::mutex kdTreeMutex_;
    //--------------------------------------------------------------------------
public:
    AdjacencyList() {
        // initialize the preprocessing parameters
        numVertices_ = 0;
        edgesAdded_ = 0;
        totalDegree_ = 0;
        hopsAllowed_ = 0;
        searchSpaceLimit_ = 0;
        readPreprocessed_ = false;
    }
    //--------------------------------------------------------------------------
    // memory and data statistics functions
    /**
     * @brief getMemoryInfo
     */
    void checkMemoryInfo() const {
        double toMegaBytes = 1048576;
        LOGG(Logger::INFO) << "[MEMORY]: number of vertices: " << numVertices_ << Logger::FLUSH;
        LOGG(Logger::INFO) << "[MEMORY]: vertices: " << getVertexMemoryInfo()/toMegaBytes << " MB" << Logger::FLUSH;
        LOGG(Logger::INFO) << "[MEMORY]: edges: " << getEdgeMemoryInfo()/toMegaBytes << " MB" << Logger::FLUSH;
        LOGG(Logger::INFO) << "[MEMORY]: total: " << getVertexMemoryInfo()/toMegaBytes+getEdgeMemoryInfo()/toMegaBytes << " MB" << Logger::FLUSH;
        //LOGG(Logger::INFO) << "[MEMORY]: number of points on geometry non-recursive " << getNumPointsToStoreNonRecursive() << Logger::FLUSH;
        //LOGG(Logger::INFO) << "[MEMORY]: number of points on geometry recursive " << getNumPointsToStoreRecursive() << Logger::FLUSH;

        checkEdgesValidity();
        dataStatistics();
    }
    /**
     * @brief getInitConfig
     * @param src
     * @param dst
     * @return
     */
    bool getInitConfig(const Point &src, const Point &dst, AlgorithmInit &initConfig) {
        NearestPointResult nearestSrc,nearestDst;
        if(!findNearestPointKdTree(src, nearestSrc)) {
            LOGG(Logger::ERROR) << "couldn't find " << src << " in kdtree" << Logger::FLUSH;
            return false;
        }
        if(!findNearestPointKdTree(dst,nearestDst)) {
            LOGG(Logger::ERROR) << "couldn't find " << dst << " in kdtree" << Logger::FLUSH;
            return false;
        }

        initConfig.setSearchResults(nearestSrc, nearestDst);

        // change destinations in case not end point
        if(!nearestSrc.isEndPoint()) {
            Vertex src1 = nearestSrc.getSrc().getVertex();
            Vertex src2 = nearestSrc.getDst().getVertex();
            Edge::EdgeType srcType = findEdgeType(src1, src2);
            initConfig.setSrcEdgeType(srcType);
        }
        if(!nearestDst.isEndPoint()) {
            Vertex dst1 = nearestDst.getSrc().getVertex();
            Vertex dst2 = nearestDst.getDst().getVertex();
            Edge::EdgeType dstType = findEdgeType(dst1, dst2);
            initConfig.setDstEdgeType(dstType);
        }

        return true;
    }

    /**
     * @brief getVertexMemoryInfo
     * @return
     */
    size_t getVertexMemoryInfo() const {
        return numVertices_*(sizeof(Vertex)+sizeof(Point));
    }

    /**
     * @brief checkForEmptyEdges
     */
    void checkEdgesValidity() const {
        LOGG(Logger::INFO) << "[MEMORY] edges vector size: " << edgesTo_.size() << Logger::FLUSH;

        assert(edgesTo_.size() == edgesFrom_.size());

        for(size_t i = 0; i < edgesTo_.size(); i++)
            if(edgesTo_[i].size() == 0 && edgesFrom_[i].size() == 0) {
                LOGG(Logger::INFO) << "[PARSE WARNING]: no edge for " << i << Logger::FLUSH;
            }
    }

    /**
     * @brief getEdgeMemoryInfo
     * @return
     */
    size_t getEdgeMemoryInfo() const {
        assert(edgesFrom_.size() == edgesFrom_.capacity());

        size_t totalMemory = 0;
        for(size_t i = 0; i < edgesFrom_.size(); i++) {
            totalMemory += edgesFrom_[i].capacity();
            //assert(edgesFrom_[i].size() == edgesFrom_[i].capacity());
        }

        for(size_t i = 0; i < edgesTo_.size(); i++)
            totalMemory += edgesTo_[i].capacity();

        totalMemory *= sizeof(EdgeBack);
        totalMemory += 2*sizeof(std::vector<EdgeBack>)*edgesTo_.capacity();
        return totalMemory;
    }

    /**
     * @brief numberOfPointsOnEdgeGeometryRecursive
     * @param src
     * @param dst
     * @return
     */
    size_t numPointsOnGeometryRecursive(VertexId src, VertexId dst) {
        size_t pointsToStore = 2;
        std::stack<pair<VertexId, VertexId> > unrollEdges;
        unrollEdges.push({src, dst});

        while(!unrollEdges.empty()) {
            pair<VertexId, VertexId> currEdg = unrollEdges.top();
            unrollEdges.pop();

            VertexId via = getViaForEdge(currEdg.first, currEdg.second);
            if(via == Vertex::NullVertexId) {
                pointsToStore++;
            }
            else {
                unrollEdges.push({currEdg.first, via});
                unrollEdges.push({via, currEdg.second});
            }
        }

        return pointsToStore;
    }

    /**
     * @brief getNumPointsToStoreRecursive
     * @param src
     * @param dst
     * @return
     */
    size_t getNumPointsToStoreNonRecursive() const {
        size_t pointsToStore = 0;
        for(VertexId id = 0; id < getNumVertices(); id++) {
            for(size_t i = 0; i < edgesFrom_[id].size(); i++) {
                pointsToStore += 2;
            }
        }
        return pointsToStore;
    }

    /**
     * @brief getNumPointsToStoreRecursive
     * @return
     */
    size_t getNumPointsToStoreRecursive() {
        size_t pointsToStore = 0;
        for(VertexId id = 0; id < getNumVertices(); id++) {
            for(size_t i = 0; i < edgesFrom_[id].size(); i++) {
                pointsToStore += numPointsOnGeometryRecursive(id, edgesFrom_[id][i].getNextId());
            }
        }

        return pointsToStore;
    }

    /**
     * @brief dataStatistics
     */
    void dataStatistics() const {
        // find out the edge with a maximal distance
        DistType maxDist = 0;
        for(VertexId id = 0; id < getNumVertices(); id++) {
            for(size_t i = 0; id < edgesTo_.size() && i < edgesTo_[id].size(); i++) {
                maxDist = max(maxDist, edgesTo_[id][i].getCost<Edge::DistanceMetric>());
            }
        }

        LOGG(Logger::INFO) << "[STATISTICS] edge with a maximal weight : " << maxDist << Logger::FLUSH;
    }
    //--------------------------------------------------------------------------
    // parsing and initialization of graph
    /**
     * @brief parse
     * @param filename
     * @param edgeFilter
     */
    bool parse(const std::string &filename) {
        SqlStream sqliteStr;
        inputFilename_ = filename;

        {
            // deserialize vertices
            URL url(filename);
            Properties props = {{"type", "sqlite"}, {"create", "0"}, {"table", SqlConsts::VERTICES_TABLE}};
            if(!sqliteStr.open(url, props)) {
                LOGG(Logger::ERROR) << "cant prepare table stream " << SqlConsts::VERTICES_TABLE << Logger::FLUSH;
                return false;
            } else {
                if(!deserializeVertices(sqliteStr, sqliteStr.getNumRows())) {
                    return false;
                }
            }
        }

        // check if there exists a preprocessed version
        fstream fstrPreprocessed(filename+".preprocessed");
        if(fstrPreprocessed.is_open()) {
            LOGG(Logger::INFO) << "reading from preprocessed file" << Logger::FLUSH;
            readPreprocessed_ = true;
            deserializeRanks(fstrPreprocessed);
            size_t numEdges = 0;
            fstrPreprocessed >> numEdges;
            deserializeEdges(fstrPreprocessed,numEdges);
        }
        else {
            // reading from preprocessed
            LOGG(Logger::INFO) << "reading from the original file" << Logger::FLUSH;
            URL url(filename);
            Properties props = {{"type", "sqlite"}, {"create", "0"}, {"table", SqlConsts::EDGES_TABLE}};
            if(!sqliteStr.open(url, props)) {
                LOGG(Logger::ERROR) << "can't prepare table stream " << SqlConsts::EDGES_TABLE << Logger::FLUSH;
                return false;
            }
            deserializeEdges(sqliteStr, sqliteStr.getNumRows());
        }

        {
            // deserialize turn restrictions
            URL url(filename);
            Properties props = {{"type", "sqlite"}, {"create", "0"}, {"table", SqlConsts::TURN_RESTRICTIONS}};
            if(!sqliteStr.open(url, props)) {
                LOGG(Logger::ERROR) << "can't prepare table stream " << SqlConsts::TURN_RESTRICTIONS << Logger::FLUSH;
            } else {
                deserializeTurnRestrictions(sqliteStr, sqliteStr.getNumRows());
            }
        }

        {
            // initialize kd tree sqlite
            URL url(filename);
            Properties props({{"type", "sqlite"}, {"create", "0"}, {"table", SqlConsts::KDTREE_ENDPT_TABLE},{"dataField","data"}});
            if(!kdTreeEndPt_.open(url, props, false)) {
                LOGG(Logger::ERROR) << "can't init endpoint kdtree with " << inputFilename_ << Logger::FLUSH;
                return false;
            }
            Properties propsNonEp({{"type", "sqlite"}, {"create", "0"}, {"table", SqlConsts::KDTREE_NON_ENDPOINT_TABLE},{"dataField","data"}});
            if(!kdTreeNonEndPt_.open(url, propsNonEp)) {
                LOGG(Logger::ERROR) << "can't init non endpoint kdtree with " << inputFilename_ << Logger::FLUSH;
                return false;
            }
        }
        {
            // open geometry index for reading
            URL url(filename);
            Properties props({{"type", "sqlite"}, {"create", "0"}});
            if(!indexGeometry_.open(url, props)) {
                LOGG(Logger::ERROR) << "can't init geometry index with " << inputFilename_ << Logger::FLUSH;
                return false;
            }
        }
        checkMemoryInfo();
        fstrPreprocessed.close();
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
        numVertices_ = 0;
        inputFilename_ = "";
        releaseMemory(edgesTo_);
        releaseMemory(edgesFrom_);
        releaseMemory(rank_);
        releaseMemory(ranked_);
        releaseMemory(unrankedNodes_);
        releaseMemory(vertexToPoint_);
        kdTreeEndPt_.close();
        kdTreeNonEndPt_.close();
        indexGeometry_.close();
        checkMemoryInfo();
    }
private:
    //--------------------------------------------------------------------------
    // serialization and deserialization routines
    /**
     * @brief deserializeVertices
     */
    template <typename T>
    bool deserializeVertices(T &iss, size_t numVertices) {
        numVertices_ = numVertices;
        vertexToPoint_.resize(numVertices);

        VertexId numRows = 0;
        while(iss.getNext()) {
            VertexPoint vpt;
            vpt.deserialize(iss);
            if(vpt.getId() >= numVertices) {
                LOGG(Logger::ERROR) << "[PARSE ERROR] vertex has too large id" << Logger::FLUSH;
                exit(EXIT_FAILURE);
            }
            vertexToPoint_[vpt.getId()] = vpt.getPoint();
            numRows++;
        }

        if(numRows != numVertices) {
            LOGG(Logger::ERROR) << "[PARSE ERROR] wrong number of rows" << Logger::FLUSH;
            return false;
        }
        return true;
    }

    /**
     * should only be called when vertices are deserialized already
     *
     * @brief deserializeEdges
     * @param iss
     * @return
     */
    template <typename T>
    void deserializeEdges(T &iss, size_t numEdges) {
        edgesTo_.resize(getNumVertices());
        edgesFrom_.resize(getNumVertices());

        for(int i = 0; i < numEdges && !iss.eof(); i++) {
            VertexId id;
            iss >> id;
            EdgeForw edgeForw;
            edgeForw.deserialize(iss);

            if(id == edgeForw.getNextId())
                LOGG(Logger::INFO) << "[PARSE WARNING] same src and dst " << id << Logger::FLUSH;
            else {
                if(id >= getNumVertices() || edgeForw.getNextId() >= getNumVertices()) {
                    LOGG(Logger::ERROR) << "[PARSE ERROR] vertex has too large id" << Logger::FLUSH;
                    exit(EXIT_FAILURE);
                }
                edgesFrom_[id].push_back(edgeForw);
                EdgeBack edgeBack(id, edgeForw.getCost<Edge::DistanceMetric>(),
                                  edgeForw.getCost<Edge::TimeMetric>(),
                                  edgeForw.getVia(), edgeForw.getType(), edgeForw.getOrigId());
                edgesTo_[edgeForw.getNextId()].push_back(edgeBack);
            }
        }

        sortEdges();

        // ensure that the size is the capacity
        for(VertexId i = 0; i < numVertices_; i++) {
            std::vector<EdgeForw> tmpForw;
            swap(tmpForw, edgesFrom_[i]);
            edgesFrom_[i] = tmpForw;

            std::vector<EdgeBack> tmpBack;
            swap(tmpBack, edgesTo_[i]);
            edgesTo_[i] = tmpBack;
        }
    }

    /**
     * should only be called when vertices and edges are deserialized already
     *
     * @brief deserializeTurnRestrictions
     * @param iss
     * @return
     */
    template <typename T>
    void deserializeTurnRestrictions(T &iss, size_t numRestrictions) {
        for(int i = 0; i < numRestrictions && iss.getNext(); i++) {
            std::string type;
            VertexId via;
            Edge::EdgeId from, to;
            iss >> from >> to >> via >> type;
        }
    }

    /**
     * @brief deserializeRanks
     */
    istream &deserializeRanks(istream &fstrPreprocessed) {
        rank_.resize(getNumVertices());
        ranked_.resize(getNumVertices(), false);

        size_t numRanked = 0;
        fstrPreprocessed >> numRanked;
        for(size_t i = 0; i < numRanked; i++) {
            VertexId id;
            VertexRank rank;

            fstrPreprocessed >> id >> rank;
            ranked_[id] = true;
            rank_[id] = rank;
        }

        return fstrPreprocessed;
    }


    /**
     * serialize edges and ranks after preprocessing with CH
     */
    void serializeEdgesAfterProcessing() const {
        LOGG(Logger::INFO) << "[NOTIFICATION] serialize edges after preprocessing" << Logger::FLUSH;
        std::string preprocessedFile = inputFilename_+".preprocessed";
        fstream oss(preprocessedFile, fstream::out);

        // figure out how many are ranked
        size_t numRanked = 0;
        for(VertexId id = 0; id < getNumVertices(); id++) {
            if(ranked_[id])
                numRanked++;
        }

        oss << numRanked << endl;

        // serialize ranks
        for(VertexId id = 0; id < getNumVertices(); id++) {
            if(ranked_[id])
                oss << id << " " << rank_[id] << endl;
        }

        // serialize edges
        size_t numEdges = 0;
        for(VertexId id = 0; id < getNumVertices(); id++) {
            numEdges += edgesFrom_[id].size();
        }
        oss << numEdges << endl;
        for(VertexId id = 0; id < getNumVertices(); id++) {
            for(const EdgeForw &edge : edgesFrom_[id]) {
                oss << id << " ";
                edge.serialize(oss);
            }
        }
        oss.close();
    }

    /**
     * @brief serializeGeometry
     * @param v1
     * @param v2
     * @param geometry
     */
    void serializePreprocessedGeometry(VertexId v1, VertexId v2, std::vector<Point> &geometry) {
        if (geometry.size() > 0) {
            indexGeometry_.insertGeometry(v1, v2, false, geometry);
        }
    }
    /**
     * @brief getNewMetricRecursive
     * @param src
     * @param dst
     * @return
     */
    DistType getNewMetricRecursive(VertexId src, VertexId dst) {
        DistType metric = 0;
        std::stack<pair<VertexId, VertexId> > unrollEdges;
        unrollEdges.push({src, dst});

        while(!unrollEdges.empty()) {
            pair<VertexId, VertexId> currEdg = unrollEdges.top();
            unrollEdges.pop();

            VertexId via = getViaForEdge(currEdg.first, currEdg.second);
            if(via == Vertex::NullVertexId) {
                metric += findEdgeForw(currEdg.first, currEdg.second)->getCost<Edge::TimeMetric>();
            }
            else {
                Edge *edge1 = findEdgeForw(currEdg.first, via);
                if(edge1->getCost<Edge::TimeMetric>() != Edge::MaxTime)
                    metric += edge1->getCost<Edge::TimeMetric>();
                else
                    unrollEdges.push({currEdg.first, via});

                Edge *edge2 = findEdgeForw(via, currEdg.second);
                if(edge2->getCost<Edge::TimeMetric>() != Edge::MaxTime)
                    metric += edge2->getCost<Edge::TimeMetric>();
                else
                    unrollEdges.push({via, currEdg.second});
            }
        }

        return metric;
    }

    /**
     * @brief serializeGeometryAfterPreprocessing
     * iterate over all edges and find all created during
     * preprocessing. Store geometries from->via->to
     * in geometry index
     */
    void serializeGeometryAfterPreprocessing() {
        LOGG(Logger::INFO) << "[NOTIFICATION] serialize geometry after preprocessing" << Logger::FLUSH;

        std::vector<pair<VertexId, VertexRank> > rankedVertices;
        for(VertexId id = 0; id < getNumVertices(); id++) {
            rankedVertices.push_back({id, getVertexRank(id)});
        }

        // sort according to ranks
        sort(rankedVertices.begin(), rankedVertices.end(), CompareRanks());

        for(VertexId id = 0; id < getNumVertices(); id++) {
            for(size_t i = 0; i < edgesFrom_[id].size(); i++) {
                if(edgesFrom_[id][i].getType() & Edge::CREATED_ON_PREPROCESSING ||
                   edgesFrom_[id][i].getVia() != Vertex::NullVertexId) {
                    std::vector<Point> first, second;
                    findGeometryForEdge(id, edgesFrom_[id][i].getNextId(), getPoint(id), first);
                    findGeometryForEdge(id, edgesFrom_[id][i].getNextId(), getPoint(id), second);

                    first.pop_back();
                    first.insert(first.end(), second.begin(), second.end());

                    serializePreprocessedGeometry(id, edgesFrom_[id][i].getNextId(), first);
                }
            }
        }
    }
    //--------------------------------------------------------------------------
    // helper functions for finding  info on edges and vertices
    /**
     * @brief findEdgeForw
     * @param src
     * @param dst
     * @return
     */
    EdgeForw* findEdgeForw(Vertex src, Vertex dst) {
        auto it = lower_bound(getOutgoingIterBegin(src), getOutgoingIterEnd(src),
                              dst, CompareVertexEdge<EdgeForw>());

        if(it != getOutgoingIterEnd(src) && it->getNextId() == dst.getId())
            return &(*it);
        else
            return 0;
    }

    /**
     * @brief findEdgeBack
     * @param src
     * @param dst
     * @return
     */
    EdgeBack* findEdgeBack(Vertex src, Vertex dst) {
        auto it = lower_bound(getIncomingIterBegin(dst), getIncomingIterEnd(dst),
                              src, CompareVertexEdge<EdgeBack>());

        if(it != getIncomingIterEnd(dst) && it->getNextId() == src.getId())
            return &(*it);
        else
            return 0;
    }

    /**
     * @brief findEdgeType
     * @param src
     * @param dst
     * @return
     */
    Edge::EdgeType findEdgeType(Vertex src, Vertex dst) {
        EdgeForw *edgeForw = findEdgeForw(src, dst);

        if(edgeForw == 0) {
            EdgeBack *edgeBack = findEdgeBack(src, dst);

            assert(edgeBack != 0);
            return edgeBack->getType();
        }
        else
            return edgeForw->getType();
    }

    /**
     * get the number of unranked neighbors
     */
    size_t numUnrankedNeighbors(Vertex id) const {
        size_t neighbors = 0;
        for(size_t i = 0; i < edgesTo_[id.getId()].size(); i++) {
            if(ranked_[edgesTo_[id.getId()][i].getNextId()])
                neighbors++;
        }

        for(size_t i = 0; i < edgesFrom_[id.getId()].size(); i++) {
            if(ranked_[edgesFrom_[id.getId()][i].getNextId()])
                neighbors++;
        }

        return neighbors;
    }

public:
    //--------------------------------------------------------------------------
    // importing new data
    /**
     * @brief importMetrics
     *
     * import time, distance or speed metric
     */
    template <typename T>
    void importMetricsStream(T &iss) {
        LOGG(Logger::INFO) << "[NOTIFICATION] importing different metrics" << Logger::FLUSH;

        while(!iss.eof()) {
            VertexId src, dst;
            iss >> src >> dst;
            DistType meters, seconds, kmh;
            iss >> meters >> seconds >> kmh;

            if(src != dst) {
                if(findEdgeForw(src, dst))
                    findEdgeForw(src, dst)->setTime(seconds);

                if(findEdgeBack(src, dst))
                    findEdgeBack(src, dst)->setTime(seconds);
            }
        }

        for(VertexId id = 0; id < getNumVertices(); id++) {
            for(size_t i = 0; i < edgesFrom_[id].size(); i++) {
                DistType metric = getNewMetricRecursive(id, edgesFrom_[id][i].getNextId());
                edgesFrom_[id][i].setTime(metric);
            }
            for(size_t i = 0; i < edgesTo_[id].size(); i++) {
                DistType metric = getNewMetricRecursive(edgesTo_[id][i].getNextId(), id);
                edgesTo_[id][i].setTime(metric);
            }
        }
    }
    //--------------------------------------------------------------------------
    // access to data
    /**
     * @brief getPoint
     * @param v
     * @return
     */
    Point getPoint(const Vertex &v) const {
        return vertexToPoint_[v.getId()];
    }

    /**
     * @brief findVertex
     * @param id
     * @return
     */
    Vertex findVertex(VertexId id) const {
        if(id < numVertices_) {
            return Vertex(id);
        } else {
            return Vertex();
        }
    }

    /**
     * @brief getVertexSize
     * @return
     */
    size_t getNumVertices() const {
        return numVertices_;
    }

    /**
     * @brief getVertexRank
     * @param v
     * @return
     */
    VertexRank getVertexRank(VertexId v) const {
        if(!ranked_[v])
            return numeric_limits<VertexRank>::max();
        return rank_[v];
    }

    /**
     * @brief isRanked
     * @param v
     * @return
     */
    bool isRanked(VertexId v) const {
        return ranked_[v];
    }

    /**
     * @brief getVertexDegree
     * @return
     */
    size_t getVertexDegree(VertexId id) const {
        return edgesTo_[id].size()+edgesFrom_[id].size();
    }

    /**
     * get outgoing edge iterators
     * @brief getOutgoingIterBegin
     * @param v
     * @return
     */
    OutgoingEdgeIter getOutgoingIterBegin(const Vertex &v) {
        return edgesFrom_[v.getId()].begin();
    }

    /**
     * @brief getOutgoingIterEnd
     * @param v
     * @return
     */
    OutgoingEdgeIter getOutgoingIterEnd(const Vertex &v) {
        return edgesFrom_[v.getId()].end();
    }

    /**
     * get incoming edges
     * @brief getIncomingIterBegin
     * @param v
     * @return
     */
    IncomingEdgeIter getIncomingIterBegin(const Vertex &v) {
        return edgesTo_[v.getId()].begin();
    }

    /**
     * @brief getIncomingIterEnd
     * @param v
     * @return
     */
    IncomingEdgeIter getIncomingIterEnd(const Vertex &v) {
        return edgesTo_[v.getId()].end();
    }

    /**
     * @brief numOutGoingEdges
     * @param id
     * @return
     */
    size_t numOutGoingEdges(VertexId id) const {
        return edgesFrom_[id].size();
    }

    /**
     * @brief numOutGoingEdges
     * @param id
     * @return
     */
    size_t numIncomingEdges(VertexId id) const {
        return edgesTo_[id].size();
    }

    /**
     * find a via vertex for an edge
     * @brief getViaForEdge
     * @param from
     * @param to
     * @return
     */
    VertexId getViaForEdge(const Vertex &from, const Vertex &to) {
        EdgeForw *edgeForw = findEdgeForw(from.getId(), to.getId());
        if(edgeForw == 0) {
            EdgeBack *edgeBack = findEdgeBack(from.getId(), to.getId());
            assert(edgeBack != 0);
            return edgeBack->getVia();
        } else {
            return edgeForw->getVia();
        }
    }

    /**
     * find nearest point using kd-tree datastructure
     * @brief findNearestPointKdTree
     * @param pt
     * @return
     */
    bool findNearestPointKdTree(const Point &pt, NearestPointResult &result) {
        // search in endpoints kdTree
        std::string endPtData;
        NearestPointResult rEndPt;
        if(!kdTreeEndPt_.findNearestVertex(pt, rEndPt, endPtData)) {
            return false;
        }
        // search in non endpoints
        std::string nonEndPtData;
        NearestPointResult rNonEndPt;
        if(!kdTreeNonEndPt_.findNearestVertex(pt, rNonEndPt, nonEndPtData)) {
            return false;
        }
        std::vector<Point> pts = {rEndPt.getTarget().getPoint(), rNonEndPt.getTarget().getPoint()};
        std::vector<std::string> ptData = {endPtData, nonEndPtData};
        std::vector<NearestPointResult> res = {rEndPt, rNonEndPt};
        size_t cId = closestPoint(pt, pts);

        result.setTarget(res[cId].getTarget());

        if (ptData[cId] != "") {
            SimpleTokenator st(ptData[cId], ' ', '\"', true);
            result.setStartId(lexical_cast<Vertex::VertexId>(st.nextToken()));
            result.setEndId(lexical_cast<Vertex::VertexId>(st.nextToken()));
        } else {
            result.setStartId(result.getTarget().getId());
            result.setEndId(result.getTarget().getId());
        }
        return true;
    }

    /**
     * Searches in vertexGeometryMap_ for the given source and target
     * and returns their geometry
     * @brief findGeometryForEdge
     * @param source vertex
     * @param target vertex
     * @param geometry to return
     * @param target point which lies on the edge
     * @return int
     */
    bool findGeometryForEdge(const VertexId source, const VertexId dest,
                             Point target, std::vector<Point> &geometry) {
        bool ret = true;
        Vertex ver1 = findVertex(source), ver2 = findVertex(dest);
        if(ver1.getId() != Vertex::NullVertexId && ver2.getId() != Vertex::NullVertexId) {
            Edge::EdgeType type = findEdgeType(ver1, ver2);
            if((type & Edge::GEOMETRY_STORED) == 0) {
                geometry = {getPoint(ver1), getPoint(ver2)};
            } else if(type & Edge::FINAL) {
                VertexId via = getViaForEdge(source, dest);
                geometry = {getPoint(ver1), getPoint(via), getPoint(ver2)};
            } else {
                geometry = {getPoint(ver1)};

                // use sqlite index to index find edge geometry
                bool oneWay = type & Edge::ONE_WAY;
                if(!indexGeometry_.findGeometry(ver1.getId(), ver2.getId(), target, oneWay, geometry))
                    ret = false;
                geometry.push_back(getPoint(ver2));
            }
        }
        return ret;
    }

    /**
     * @brief findOrigWayId
     * @param s
     * @param d
     * @return
     */
    bool findOrigWayId(VertexId s, VertexId d, Edge::EdgeId &origId) {
        // after preprocessing edges in low to high rank direction are removed
        // so we try every possible option
        bool good = false;
        {
            EdgeForw *edge = findEdgeForw(s, d);
            if(edge != 0) {
                origId = edge->getOrigId();
                good = true;
            }
        }
        {
            EdgeForw *edge = findEdgeForw(d, s);
            if(edge != 0) {
                origId = edge->getOrigId();
                good = true;
            }
        }
        {
            EdgeBack *edge = findEdgeBack(s, d);
            if(edge != 0) {
                origId = edge->getOrigId();
                good = true;
            }
        }
        {
            EdgeBack *edge = findEdgeBack(d, s);
            if(edge != 0) {
                origId = edge->getOrigId();
                good = true;
            }
        }
        return good;
    }

    /**
     * @brief findOrigWayIds
     * @param path
     * @param wayIds
     * @return
     */
    bool findOrigWayIds(const std::vector<VertexId> &path, std::vector<Edge::EdgeId> &wayIds) {
        for(int i = 0; i+1 < path.size(); i++) {
            // after preprocessing edges in low to high rank direction are removed
            // so we try every possible option
            Edge::EdgeId origId = Vertex::NullVertexId;
            if(findOrigWayId(path[i], path[i+1], origId))
                wayIds.push_back(origId);
            else
                wayIds.push_back(Vertex::NullVertexId);
        }
        return true;
    }

    /**
     * sort edges for fast searching afterwards
     * @brief sortEdges
     */
    void sortEdges() {
        for(VertexId i = 0; i < getNumVertices(); i++) {
            auto itOutBegin = edgesFrom_[i].begin();
            auto itOutEnd = edgesFrom_[i].end();
            sort(itOutBegin, itOutEnd, CompareEdge<EdgeForw>());

            auto it = unique(itOutBegin, itOutEnd, CompareEdgeEqual<EdgeForw>());
            edgesFrom_[i].resize(it -itOutBegin);
        }

        for(VertexId i = 0; i < getNumVertices(); i++) {
            auto itInBegin = edgesTo_[i].begin();
            auto itInEnd = edgesTo_[i].end();
            sort(itInBegin, itInEnd, CompareEdge<EdgeBack>());

            auto it = unique(itInBegin, itInEnd, CompareEdgeEqual<EdgeBack>());
            edgesTo_[i].resize(it -itInBegin);
        }
    }

    /**
     * remove all edges from higher to lower rank
     * @brief pruneEdgesRank
     */
    void pruneEdgesRank() {
        LOGG(Logger::INFO) << "[NOTIFICATION] pruning edges and ranks" << Logger::FLUSH;

        size_t prunedEdgesCounter = 0, edgesCounter = 0;
        for(VertexId id = 0; id < getNumVertices(); id++) {
            std::vector<EdgeForw> tmpForwEdges;
            for(size_t j = 0; j < edgesFrom_[id].size(); j++) {
                if(getVertexRank(id) <= getVertexRank(edgesFrom_[id][j].getNextId())) {
                    tmpForwEdges.push_back(edgesFrom_[id][j]);
                }
            }
            edgesCounter += edgesFrom_[id].size();
            prunedEdgesCounter += edgesFrom_[id].size()-tmpForwEdges.size();
            edgesFrom_[id].swap(tmpForwEdges);

            std::vector<EdgeBack> tmpBackEdges;
            for(size_t j = 0; j < edgesTo_[id].size(); j++) {
                if(getVertexRank(id) <= getVertexRank(edgesTo_[id][j].getNextId())) {
                    tmpBackEdges.push_back(edgesTo_[id][j]);
                }
            }
            edgesCounter += edgesTo_[id].size();
            prunedEdgesCounter += edgesTo_[id].size()-tmpBackEdges.size();
            edgesTo_[id].swap(tmpBackEdges);
        }

        // destroy rank information
        releaseMemory(ranked_);
        releaseMemory(rank_);
        LOGG(Logger::INFO) << "[NOTIFICATION] pruned " << 100.0*(double)prunedEdgesCounter/(double)edgesCounter << "% edges" << Logger::FLUSH;
    }

    /**
     * merge two edge sequences
     */
    void mergeEdgeBackSequences(VertexId id, const std::vector<EdgeBack> &newBackEdges) {
        size_t oldSize = edgesTo_[id].size();
        edgesTo_[id].insert(edgesTo_[id].end(), newBackEdges.begin(), newBackEdges.end());
        inplace_merge(edgesTo_[id].begin(), edgesTo_[id].begin()+oldSize,
                      edgesTo_[id].end(), CompareEdge<EdgeBack>());
    }

    /**
     * merge two edge sequences
     */
    void mergeEdgeForwSequences(VertexId id, const std::vector<EdgeForw> &newForwEdges) {
        size_t oldSize = edgesFrom_[id].size();
        edgesFrom_[id].insert(edgesFrom_[id].end(), newForwEdges.begin(), newForwEdges.end());
        inplace_merge(edgesFrom_[id].begin(), edgesFrom_[id].begin()+oldSize,
                      edgesFrom_[id].end(), CompareEdge<EdgeForw>());
    }

    /**
     * see if we already processed this pair
     * @brief findWitness
     * @param from
     * @return
     */
    DistType findWitness(VertexId from, VertexId to) {
        auto it = oneHopWitness_[from].find(to);
        if(it != oneHopWitness_[from].end())
            return it->second;
        return numeric_limits<DistType>::max();
    }

    typedef std::vector<SearchResultLocal> LocalDijkstraSearchResult;
    typedef std::vector<std::vector<bool> > DijstraStarted;
    typedef std::vector<std::vector<DistType> > NewDistanceMap;

    /**
     * @brief recalculateParameters
     */
    void recalculateParameters(const Vertex &currVert,
                               const DijstraStarted &wasDijkstaStarted,
                               const LocalDijkstraSearchResult &searchResult,
                               const NewDistanceMap &newDistances,
                               size_t &shortcutsCreated, size_t &totalSearchSpace) {
        auto itInBegin = getIncomingIterBegin(currVert);
        auto itInEnd = getIncomingIterEnd(currVert);

        for(int i = 0; itInBegin != itInEnd; ++itInBegin, i++) {
            if(ranked_[itInBegin->getNextId()]) break;

            totalSearchSpace += searchResult[i].getSearchSpace();

            auto itOutBegin = getOutgoingIterBegin(currVert);
            auto itOutEnd = getOutgoingIterEnd(currVert);

            for(int j = 0; itOutBegin != itOutEnd; ++itOutBegin, j++) {
                // check if dest vertex is not ranked
                if(ranked_[itOutBegin->getNextId()]) break;

                // dijkstra was started
                if(wasDijkstaStarted[i][j]) {
                    if(searchResult[i].getTargetDistFor(itOutBegin->getNextId()) > newDistances[i][j]) {
                        shortcutsCreated++;
                    }
                }
                // dijkstra wasn't started only because we saw this edge already or
                // edge was already there and it wasn't updated
                else {
                    if(findWitness(itInBegin->getNextId(), itOutBegin->getNextId()) == newDistances[i][j]) {
                        shortcutsCreated++;
                    }
                }
            }
        }
    }

    /**
     * @brief createForwardEdges
     */
    void createForwardEdges(const Vertex &currVert,
                            const DijstraStarted &wasDijkstaStarted,
                            const LocalDijkstraSearchResult &searchResult,
                            const NewDistanceMap &newDistances) {
        std::vector<std::thread> threads;

        // merge forward edges
        std::vector<std::vector<EdgeForw> > newForwEdges(edgesTo_[currVert.getId()].size(),
                                              std::vector<EdgeForw>());

        auto itInBegin = getIncomingIterBegin(currVert);
        auto itInEnd = getIncomingIterEnd(currVert);

        for(int i = 0; itInBegin != itInEnd; ++itInBegin, i++) {
            VertexId toId = itInBegin->getNextId();
            if(ranked_[toId]) break;

            auto itOutBegin = getOutgoingIterBegin(currVert);
            auto itOutEnd = getOutgoingIterEnd(currVert);

            for(int j = 0; itOutBegin != itOutEnd; ++itOutBegin, j++) {
                VertexId fromId = itOutBegin->getNextId();
                if(isRanked(fromId)) break;

                //check if dest vertex is not ranked
                if(fromId != toId) {
                    // dijkstra was started, hence we
                    if(wasDijkstaStarted[i][j]) {
                        if (searchResult[i].getTargetDistFor(fromId) > newDistances[i][j]) {
                            Vertex to = itOutBegin->getNext();
                            EdgeForw edgFrom(to, newDistances[i][j], newDistances[i][j], currVert.getId(),
                                             Edge::CREATED_ON_PREPROCESSING);
                            newForwEdges[i].push_back(edgFrom);
                            edgesAdded_++;

                            // update witness
                            oneHopWitness_[toId][fromId] = newDistances[i][j];
                        }
                    } else {
                        // saw this edge already
                        DistType witness = findWitness(itInBegin->getNextId(), itOutBegin->getNextId());
                        if(witness == newDistances[i][j]) {
                            Vertex to = itOutBegin->getNext();
                            EdgeForw edgFrom(to, witness, witness, currVert.getId(), Edge::CREATED_ON_PREPROCESSING);
                            newForwEdges[i].push_back(edgFrom);
                            edgesAdded_++;
                        }
                    }
                }
            }
            Vertex from = itInBegin->getNext();
            threads.push_back(std::thread(std::bind(&AdjacencyList::mergeEdgeForwSequences, this,
                                                    from.getId(), std::ref(newForwEdges[i]))));
        }
        for(int j=0;j<(int)threads.size();++j)
            threads[j].join();
    }

    /**
     * @brief createBackwardEdges
     */
    void createBackwardEdges(const Vertex &currVert,
                             const DijstraStarted &wasDijkstaStarted,
                             const LocalDijkstraSearchResult &searchResult,
                             const NewDistanceMap &newDistances) {
        std::vector<std::thread> threads;

        // merge backward edges
        std::vector<std::vector<EdgeBack> > newBackEdges(edgesFrom_[currVert.getId()].size(),
                                               std::vector<EdgeBack>());

        auto itOutBegin = getOutgoingIterBegin(currVert);
        auto itOutEnd = getOutgoingIterEnd(currVert);

        for(int j = 0; itOutBegin != itOutEnd; ++itOutBegin, j++) {
            // check if src vertex is not ranked
            VertexId fromId = itOutBegin->getNextId();
            if(ranked_[fromId]) break;

            auto itInBegin = getIncomingIterBegin(currVert);
            auto itInEnd = getIncomingIterEnd(currVert);

            for(int i = 0; itInBegin != itInEnd; ++itInBegin, i++) {
                VertexId toId = itInBegin->getNextId();
                if(isRanked(toId)) break;
                // check if src vertex is not ranked
                if(fromId != toId) {
                    if(wasDijkstaStarted[i][j]) {
                        if(searchResult[i].getTargetDistFor(fromId) > newDistances[i][j]) {
                            Vertex from = itInBegin->getNext();
                            EdgeBack edgTo(from, newDistances[i][j], newDistances[i][j], currVert.getId(),
                                           Edge::CREATED_ON_PREPROCESSING);
                            newBackEdges[j].push_back(edgTo);
                            edgesAdded_++;

                            // update witness
                            oneHopWitness_[fromId][toId] = newDistances[i][j];
                        }
                    } else {
                        // saw this optimal path before
                        DistType witness = findWitness(itOutBegin->getNextId(), itInBegin->getNextId());
                        if(witness == newDistances[i][j]) {
                            Vertex from = itInBegin->getNext();
                            EdgeBack edgTo(from, witness, witness, currVert.getId(), Edge::CREATED_ON_PREPROCESSING);
                            newBackEdges[j].push_back(edgTo);
                            edgesAdded_++;
                        }
                    }
                }
            }

            Vertex to = itOutBegin->getNext();
            threads.push_back(std::thread(std::bind(&AdjacencyList::mergeEdgeBackSequences, this,
                                                    to.getId(), std::ref(newBackEdges[j]))));
        }

        for(int j=0;j<(int)threads.size();++j)
            threads[j].join();
    }

    /**
     * either simulates the contraction of the node or actually contracts it
     * depending on addEdges value
     */
    template<typename Metric = Edge::DistanceMetric>
    void processNode(Vertex currVert, const LocalGraph<AdjacencyList> &graph,
                     VertexProcResult &procResult, bool addEdges = false) {

        std::vector<std::thread> threads;

        // accumulating parameters in case we want to create edges later
        size_t numIn = numIncomingEdges(currVert.getId()),
               numOut = numOutGoingEdges(currVert.getId());
        LocalDijkstraSearchResult searchResult(numIn);
        DijstraStarted wasDijkstaStarted(numIn, std::vector<bool>(numOut, 0));
        NewDistanceMap newDistances(numIn, std::vector<DistType>(numOut, 0));

        // iterate over all possible pairs of incoming and outgoing vertices
        //check if dest vertex is not ranked

        auto itInBegin = getIncomingIterBegin(currVert);
        auto itInEnd = getIncomingIterEnd(currVert);

        for(int i = 0; itInBegin != itInEnd; ++itInBegin, i++) {
            if(ranked_[itInBegin->getNextId()]) break;

            DistType maxDistance = 0;

            // accumulate target vertices and do a one time pass
            // and try to reach all at once
            unordered_set<VertexId> targetVertices(numOutGoingEdges(currVert.getId()));
            searchResult[i] = SearchResultLocal();

            // for all pairs of outgoing and incoming edges
            auto itOutBegin = getOutgoingIterBegin(currVert);
            auto itOutEnd = getOutgoingIterEnd(currVert);

            for(int j = 0; itOutBegin != itOutEnd; ++itOutBegin, j++) {
                if(ranked_[itOutBegin->getNextId()]) break;

                if(itInBegin->getNextId() == itOutBegin->getNextId())
                    continue;

                // find out if there was already an edge between the pair of vertices
                // forward edge
                auto edgeOut = lower_bound(getOutgoingIterBegin(itInBegin->getNext()),
                                           getOutgoingIterEnd(itInBegin->getNext()),
                                           itOutBegin->getNext(), CompareVertexEdgeUnranked<EdgeForw>(this));

                // if the edge is not found
                if(edgeOut->getNextId() != itOutBegin->getNextId())
                    edgeOut = getOutgoingIterEnd(itInBegin->getNext());

                // find out if there was already an edge between the pair of vertices
                // backward edge
                auto edgeIn = lower_bound(getIncomingIterBegin(itOutBegin->getNext()),
                                          getIncomingIterEnd(itOutBegin->getNext()),
                                          itInBegin->getNext(), CompareVertexEdgeUnranked<EdgeBack>(this));

                // if the edge is not found
                if(edgeIn->getNextId() != itInBegin->getNextId())
                    edgeIn = getIncomingIterEnd(itOutBegin->getNext());

                // cost of the new edge
                DistType newCost = itOutBegin->template getCost<Metric>()+itInBegin->template getCost<Metric>();
                maxDistance = max(maxDistance, newCost);

                if(edgeOut != getOutgoingIterEnd(itInBegin->getNext())) {
                    assert(edgeIn != getIncomingIterEnd(itOutBegin->getNext()));

                    if(addEdges && edgeOut->template getCost<Metric>() >= newCost) {
                        // update forward edge
                        edgeOut->setVia(currVert.getId());
                        edgeOut->setDist(newCost);

                        // update backward edge
                        edgeIn->setVia(currVert.getId());
                        edgeIn->setDist(newCost);
                    }
                }
                // if there wasn't any edge, create new one
                else {
                    // if the distance is shorter than new cost, don't execute dijkstra on
                    // this target vertex
                    if(findWitness(itInBegin->getNextId(), itOutBegin->getNextId()) > newCost) {
                        wasDijkstaStarted[i][j] = true;
                        targetVertices.insert(itOutBegin->getNextId());
                    }
                    newDistances[i][j] = newCost;
                }
            }

            if(targetVertices.size() > 0) {
                Vertex from = itInBegin->getNext();

                // prepare Dijkstra configuration
                LocalGraph<AdjacencyList>::DijkstraInit initConfig;
                initConfig.setStoppingDistance(maxDistance);
                initConfig.setIgnoredVertex(currVert.getId());
                initConfig.setTargetVertices(targetVertices);
                initConfig.setSearchSpace(searchSpaceLimit_);
                initConfig.setHopNumber(hopsAllowed_);
                initConfig.setSrc(from);

                // dispatch Dijkstra
                threads.push_back(std::thread(std::bind(&LocalGraph<AdjacencyList>::shortestPathDijkstraLocal,
                                                        std::ref(graph), initConfig, std::ref(searchResult[i]))));
            }
        }

        for(int j=0;j<(int)threads.size();++j)
            threads[j].join();

        recalculateParameters(currVert, wasDijkstaStarted, searchResult, newDistances,
                              procResult.totalShortcuts_, procResult.totalSearchSpace_);

        if(addEdges) {
            createForwardEdges(currVert, wasDijkstaStarted, searchResult, newDistances);
            createBackwardEdges(currVert, wasDijkstaStarted, searchResult, newDistances);
        }
    }


    /**
     * @brief getHeuristic
     * @param id
     * @return
     */
    HeuristicVal getHeuristic(Vertex id, size_t shortcutsAdded, size_t searchSpace) {
        size_t neighborsContracted = 0;
        for(size_t i = 0; i < edgesTo_[id.getId()].size(); i++) {
            if(ranked_[edgesTo_[id.getId()][i].getNextId()])
                neighborsContracted++;
        }

        for(size_t i = 0; i < edgesFrom_[id.getId()].size(); i++) {
            if(ranked_[edgesFrom_[id.getId()][i].getNextId()])
                neighborsContracted++;
        }

        return 190*(8*shortcutsAdded-(getVertexDegree(id.getId())))+120*(neighborsContracted) + searchSpace;
    }
    /**
     * modify the number of hops allowed according to the average degree
     * in the graph
     */
    void modifyNumberOfHops() {
        double avgDegree = (double)totalDegree_/(double)getNumVertices();
        hopsAllowed_ = 1;
        if(avgDegree >= 3.3) {
            hopsAllowed_ = 2;
        }
        if(avgDegree >= 8.0) {
            hopsAllowed_ = 3;
        }
        if(avgDegree >= 10.0) {
            hopsAllowed_ = 5;
        }
    }

    /**
     * @brief updateNeighborEdges
     * @param currVert
     */
    void updateNeighborEdges(VertexId currVert) {
        auto itInBegin = getIncomingIterBegin(currVert);
        auto itInEnd = getIncomingIterEnd(currVert);

        for(;itInBegin != itInEnd; ++itInBegin) {
            Vertex neighbor = itInBegin->getNextId();
            std::sort(getOutgoingIterBegin(neighbor), getOutgoingIterEnd(neighbor),
                      CompareEdgeUnranked<EdgeForw>(this));
        }

        auto itOutBegin = getOutgoingIterBegin(currVert);
        auto itOutEnd = getOutgoingIterEnd(currVert);

        for(;itOutBegin != itOutEnd; ++itOutBegin) {
            Vertex neighbor = itOutBegin->getNextId();
            std::sort(getIncomingIterBegin(neighbor), getIncomingIterEnd(neighbor),
                      CompareEdgeUnranked<EdgeBack>(this));
        }
    }
    /**
     * preprocess with constraction hierarchies
     * @brief preprocessCH
     */
    void preprocess() {
        // we have found a preprocessed file
        if(readPreprocessed_)
            return;

        Timer timer;

        // create a temporary graph for computations
        LocalGraph<AdjacencyList> graph;
        graph.setModel(this);

        // prepate the rank datastructure
        rank_.resize(getNumVertices());
        ranked_.resize(getNumVertices(), false);

        // statistics
        size_t edgesUpdated = 0, maxIn = 0, maxOut = 0, reinsertedNodes = 0, currRank = 0;
        totalDegree_ = 0;

        // initialize the total degree of vertices
        for(VertexId currVert = 0; currVert < vertexToPoint_.size(); ++currVert) {
            totalDegree_ += getVertexDegree(currVert);
        }

        // the node contraction priority
        Heap<VertexId, HeuristicVal> contractOrder;
        contractOrder.initResizeHeap(getNumVertices());

        // the set of uranked nodes
        unrankedNodes_ = UnrankedVertexSet();

        // initialize witness
        oneHopWitness_ = WitnessMap(getNumVertices());

        // assign ranks to vertices
        modifyNumberOfHops();
        searchSpaceLimit_ = 200;
        for(VertexId currVert = 0; currVert < vertexToPoint_.size(); ++currVert) {
            VertexProcResult vertProc;
            unrankedNodes_.insert(currVert);
            processNode(currVert, graph, vertProc);
            HeuristicVal heuristic = getHeuristic(currVert, vertProc.totalShortcuts_,
                                               vertProc.totalSearchSpace_);
            contractOrder.pushHeap(currVert, heuristic);
        }
        timer.stop();
        LOGG(Logger::INFO) << "[PREPROCESSING CH]: ranking done in" << timer.getElapsedTimeSec() << Logger::FLUSH;

        timer.start();
        while(!contractOrder.empty()) {
            auto currMinOrder = contractOrder.topHeap();
            contractOrder.popHeapFloyd();

            Vertex currVert = currMinOrder.first;

            // maximum degrees of a node
            maxIn = max(maxIn, numIncomingEdges(currVert.getId()));
            maxOut = max(maxOut, numOutGoingEdges(currVert.getId()));

            if(currRank % 10000 == 0) {
                LOGG(Logger::INFO) << "[PREPROCESSING CH] current vertex: " << currRank << "out of"<<getNumVertices()<< Logger::FLUSH;
                LOGG(Logger::INFO) << "[PREPROCESSING CH] max indegree: " << maxIn << "max outdegree:" << maxOut << Logger::FLUSH;
                LOGG(Logger::INFO) << "[PREPROCESSING CH] edges added: " << edgesAdded_ << Logger::FLUSH;
                LOGG(Logger::INFO) << "[PREPROCESSING CH] nodes reinserted: " << reinsertedNodes << Logger::FLUSH;
                LOGG(Logger::INFO) << "[PREPROCESSING CH] avg degree: " << (double)totalDegree_/(double)getNumVertices() << Logger::FLUSH;
            }

            // modify the number of hops allowed and the search space
            modifyNumberOfHops();


            VertexProcResult vertProc;
            processNode(currVert, graph, vertProc, true);


            // update the structures
            rank_[currVert.getId()] = currRank;
            ranked_[currVert.getId()] = true;
            unrankedNodes_.erase(currVert.getId());
            updateNeighborEdges(currVert.getId());

            // update statistics
            totalDegree_ += 2*vertProc.totalShortcuts_;
            currRank++;

            // update the orders of the neighbors
            unordered_set<Vertex> neighbors;
            auto itOutBegin = getOutgoingIterBegin(currVert);
            auto itOutEnd = getOutgoingIterEnd(currVert);
            for(; itOutBegin != itOutEnd; ++itOutBegin) {
                if(!isRanked(itOutBegin->getNextId()))
                    neighbors.insert(itOutBegin->getNext());
            }

            auto itInBegin = getIncomingIterBegin(currVert);
            auto itInEnd = getIncomingIterEnd(currVert);
            for(; itInBegin != itInEnd; ++itInBegin) {
                if(!isRanked(itInBegin->getNextId()))
                    neighbors.insert(itInBegin->getNext());
            }

            // simulate the contraction for all neighbors
            for(const Vertex &neighbor: neighbors) {
                VertexProcResult vertProcLocal;
                processNode(neighbor, graph, vertProcLocal);
                HeuristicVal heuristic = getHeuristic(neighbor.getId(), vertProcLocal.totalShortcuts_,
                                                      vertProcLocal.totalSearchSpace_);
                contractOrder.changeKey(neighbor.getId(), heuristic);
            }
            releaseMemory(oneHopWitness_[currVert.getId()]);
        }

        timer.stop();
        LOGG(Logger::INFO) << "[PREPROCESSING CH] Done in " << timer.getElapsedTimeSec() << Logger::FLUSH;

        sortEdges();
        LOGG(Logger::INFO) << "[PREPROCESSING CH] total edges added: " << edgesAdded_ << Logger::FLUSH;
        LOGG(Logger::INFO) << "[PREPROCESSING CH] total edges updated: " << edgesUpdated << Logger::FLUSH;

        serializeEdgesAfterProcessing();
        pruneEdgesRank();
    }
};
