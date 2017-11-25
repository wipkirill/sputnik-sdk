#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/GraphCore/Heap.h>
#include <UrbanLabs/Sdk/GraphCore/STLHeap.h>
#include <UrbanLabs/Sdk/Utils/Timer.h>
#include <UrbanLabs/Sdk/Utils/Logger.h>
#include <UrbanLabs/Sdk/Utils/FileSystemUtil.h>

//#define GRAPH_DEBUG
template<class GraphModel>
class BaseGraph {
public:
    typedef typename GraphModel::EdgeType EdgeType;
    typedef typename GraphModel::VertexType VertexType;
    typedef typename EdgeType::EdgeId EdgeId;
    typedef typename VertexType::VertexId VertexId;

    typedef typename GraphModel::DistType DistType;
    typedef typename GraphModel::NearestPointResult NearestPointResult;
    typedef typename GraphModel::SearchResult SearchResult;
    typedef typename GraphModel::AlgorithmInit DijkstraInit;
    typedef typename GraphModel::AlgorithmState DijkstraState;
protected:
    // object representing edges, vertices, geometry storage of the graph
    GraphModel *gModel_;
public:
    /**
     * @brief BaseGraph
     */
    BaseGraph() : gModel_(0) {}

    /**
     * @brief setModel
     * @param model
     */
    void setModel(GraphModel *model) {
        gModel_ = model;
    }

    /**
     * @brief getModel
     * @return
     */
    GraphModel *getModel() {
        return gModel_;
    }

    /**
     * parse the graph from file
     * @brief parseGraph
     * @param filename
     * @param edgeFilter
     */
    bool parseGraph(const std::string &filename) {
        // cleanup previous storage
        if(gModel_ == 0)
            gModel_ = new GraphModel();
        else
            gModel_->unload();

        // parse new data
        if(!gModel_->parse(filename)) {
            gModel_->unload();
            return false;
        }

        LOGG(Logger::WARNING) << "The graph is not preprocessed." << Logger::FLUSH;
        //gModel_->preprocess();
        return true;
    }

    /**
     * @brief unloadGraph
     */
    void unloadGraph() {
        if(gModel_ != 0) {
            gModel_->unload();
            delete gModel_;
            gModel_ = 0;
        }
    }

    /**
     * @brief printPath
     * @param path
     */
    void printPath(std::vector<VertexId> &path) {
        for(size_t i = 0; i < path.size(); i++) {
            LOGG(Logger::INFO) << path[i] << "->";
        }
        LOGG(Logger::INFO) << Logger::FLUSH;
    }

    /**
     * unrolls the path found by bidirectional dijkstra
     * @brief unrollPathBiDir
     * @param s
     * @param c
     * @param d
     * @param nextMap
     * @param prevMap
     * @return
     */
    std::vector<VertexId> unrollPathBiDir(VertexId s, VertexId c, VertexId d,
                                     DijkstraState &stateB, DijkstraState &stateF) {
        // unroll before and after the common vertex
        std::vector<VertexId> pathForw = stateF.unrollPath(s, c);
        std::vector<VertexId> pathBack = stateB.unrollPath(d, c, true);

        // pop the common point and the end
        pathForw.pop_back();

        // merge two paths
        pathForw.insert(pathForw.end(), pathBack.begin(), pathBack.end());
        return pathForw;
    }

    /**
     * @brief sameWay
     * @param start
     * @param end
     * @return
     */
    bool areOnSameWay(const NearestPointResult &start, const NearestPointResult &end) const {
        bool sameWay = false;
        // when both are endpoints the points have to have same id
        if(start.isEndPoint() && end.isEndPoint()) {
            return start.getSrc().getId() == end.getDst().getId();
        } else {
            if((start.getSrc().getId() == end.getSrc().getId() || start.isEndPoint()) &&
               (start.getDst().getId() == end.getDst().getId() || end.isEndPoint())) {
                sameWay = true;
            } else if((start.getSrc().getId() == end.getDst().getId() || start.isEndPoint()) &&
                      (start.getDst().getId() == end.getSrc().getId() || end.isEndPoint())) {
                sameWay = true;
            }
        }
        return sameWay;
    }

    /**
     * returns a vector of points on path given by vertex id's
     * @brief findMultiLinesFromPath
     * @param path
     * @return
     */
    void findMultiLinesFromPath(const NearestPointResult &start, const NearestPointResult &end,
                                const std::vector<VertexId> &path, std::vector<std::vector<Point> > &multiLines) {

        // no path was found
        if(path.size() == 0) {
            return;
        }
        // same way or same source and destination
        // if same way occurs, the path size is always 1
        else if(path.size() == 1) {
            bool sameWay = areOnSameWay(start, end);
            if(sameWay) {
                bool sameVertex = start.isEndPoint() && end.isEndPoint();
                if(!sameVertex) {
                    std::vector<Point> segment;
                    if(!start.isEndPoint())
                        gModel_->findGeometryForEdge(start.getSrc().getId(), start.getDst().getId(),
                                                     start.getTarget().getPoint(), segment);
                    else
                        gModel_->findGeometryForEdge(end.getSrc().getId(), end.getDst().getId(),
                                                     end.getTarget().getPoint(), segment);
                    // find id of start and end in the segment
                    int endIndex = closestPoint(end.getTarget().getPoint(), segment);
                    int startIndex = closestPoint(start.getTarget().getPoint(), segment);

                    int initSize = segment.size();
                    if(endIndex < startIndex) {
                        reverse(segment.begin(), segment.end());
                        startIndex = initSize-startIndex-1;
                        endIndex = initSize-endIndex-1;
                    }

                    for(int i = 0; i < endIndex-startIndex+1; i++)
                        segment[i] = segment[i+startIndex];

                    for(int i = 0; i < initSize-endIndex+startIndex-1; i++)
                        segment.pop_back();
                    multiLines.push_back(segment);
                } else {
                    // if its the same vertex, out only one point to the geometry
                    multiLines.push_back({start.getTarget().getPoint()});
                }
                return;
            } else {
                // two ways joined on one endpoint, both points are not endpoints
                assert(!start.isEndPoint() && !end.isEndPoint());
            }
        }

        // the first segment
        if (!start.isEndPoint()) {
            std::vector<Point> startSegment;
            gModel_->findGeometryForEdge(start.getSrc().getId(), start.getDst().getId(),
                                        start.getTarget().getPoint(),startSegment);

            //find id of target point in startSegment
            int pointIndex = closestPoint(start.getTarget().getPoint(),startSegment);

            //path begins from start edge
            if (path[0] == start.getSrc().getId()) {
                //print all points from target to start
                std::vector<Point> firstSegment(startSegment.begin(), startSegment.begin()+pointIndex+1);
                reverse(firstSegment.begin(), firstSegment.end());
                multiLines.push_back(firstSegment);
            } else {
                //print all points from target to end
                std::vector<Point> firstSegment(startSegment.begin()+pointIndex, startSegment.end());
                multiLines.push_back(firstSegment);
            }
        }
        // the middle segment
        for (size_t i = 0, j  = 1; i < path.size() && j < path.size(); i++, j++) {
            std::vector<Point> geometry;
            gModel_->findGeometryForEdge(path[i], path[j], gModel_->getPoint(path[i]), geometry);
            multiLines.push_back(geometry);
        }
        // the last segment
        if (!end.isEndPoint()) {
            std::vector<Point> endSegment;
            gModel_->findGeometryForEdge(end.getSrc().getId(), end.getDst().getId(),
                                        end.getTarget().getPoint(), endSegment);

            //find id of target point in startSegment
            int pointIndex = closestPoint(end.getTarget().getPoint(),endSegment);

            //path ends on start point
            if (path[path.size()-1] == end.getSrc().getId()) {
                std::vector<Point> lastSegment(endSegment.begin(), endSegment.begin()+pointIndex+1);
                multiLines.push_back(lastSegment);
            } else if(path[path.size()-1] == end.getDst().getId()){
                std::vector<Point> lastSegment(endSegment.begin()+pointIndex, endSegment.end());
                reverse(lastSegment.begin(), lastSegment.end());
                multiLines.push_back(lastSegment);
            } else {
                LOGG(Logger::ERROR) << "Something is wrong!" << Logger::FLUSH;
                assert(false);
            }
        }
    }

    /**
     * @brief findOrigWayIds
     * @param path
     * @param wayIds
     * @return
     */
    bool findOrigWayIds(const SearchResult &result, std::vector<Edge::EdgeId> &wayIds) const {
        NearestPointResult start = result.getSrc(), end = result.getDst();
        if(areOnSameWay(start, end)) {
            // cannot determine the way
            if(start.isEndPoint() && end.isEndPoint()) {
                wayIds.push_back(VertexType::NullVertexId);
            } else {
                // path.size() == 1
                VertexId from, to;
                if(!start.isEndPoint())
                    from = start.getSrc().getId(), to = start.getDst().getId();
                else
                    from = end.getSrc().getId(), to = start.getDst().getId();

                Edge::EdgeId origId;
                if(!gModel_->findOrigWayId(from, to, origId))
                    wayIds.push_back(VertexType::NullVertexId);
                else
                    wayIds.push_back(origId);
            }
        } else {
            if(!start.isEndPoint()) {
                Edge::EdgeId origId;
                VertexId from = start.getSrc().getId(), to = start.getDst().getId();
                if(!gModel_->findOrigWayId(from, to, origId))
                    wayIds.push_back(VertexType::NullVertexId);
                else
                    wayIds.push_back(origId);
            }
            std::vector<Edge::EdgeId> origIds;
            // case when both are not endpoints but lie on neighboring ways
            if(result.getPath().size() != 1) {
                gModel_->findOrigWayIds(result.getPath(), origIds);
                wayIds.insert(wayIds.end(), origIds.begin(), origIds.end());
            }
            if(!end.isEndPoint()) {
                Edge::EdgeId origId;
                VertexId from = end.getSrc().getId(), to = end.getDst().getId();
                if(!gModel_->findOrigWayId(from, to, origId))
                    wayIds.push_back(VertexType::NullVertexId);
                else
                    wayIds.push_back(origId);
            }
        }
        return true;
    }

    /**
     * @brief findNearestPoint
     * @param pt
     * @return
     */
    bool findNearestPoint(const Point &pt, NearestPointResult &result) {
        return gModel_->findNearestPointKdTree(pt, result);
    }
};

template<class GraphModel>
class LocalGraph : public BaseGraph<GraphModel> {
public:
    typedef typename GraphModel::VertexType VertexType;
    typedef typename VertexType::VertexId VertexId;
    typedef typename GraphModel::EdgeType EdgeType;
    typedef typename EdgeType::EdgeId EdgeId;
    typedef typename GraphModel::DistType DistType;

    typedef typename GraphModel::NearestPointResult NearestPointResult;
    typedef typename GraphModel::SearchResultLocal SearchResultLocal;
    typedef typename GraphModel::AlgorithmInitLocal DijkstraInit;
    typedef typename GraphModel::AlgorithmState DijkstraState;
public:
    /**
     * @brief LocalGraph
     */
    LocalGraph() : BaseGraph<GraphModel>() {;}

    /**
     * shortest path using local Dijkstra step used for constructing contraction hierarchies
     * @brief shortestPathDijkstraLocal
     * @param s
     * @param d
     * @param path
     * @return
     */
    DistType shortestPathDijkstraLocal(DijkstraInit &initConfig, SearchResultLocal &result) const {
#ifdef GRAPH_DEBUG
        size_t vertices = 0;
#endif
        typedef typename GraphModel::AlgorithmStateLocal DijkstraStateLocal;

        DijkstraStateLocal state(this->gModel_);
        state.init(initConfig);

        // the list of targets to be visited
        std::unordered_set<VertexId> targetsToVisit(initConfig.targetVertices_.begin(),
                                                    initConfig.targetVertices_.end());

#ifdef GRAPH_DEBUG
        DistType oldDistance = 0;
#endif

        size_t currSearchSpace = 0;
        while(!state.isDone()) {
#ifdef GRAPH_DEBUG
            vertices++;
#endif

            // find the current closest vertex
            auto currMin = state.getNextVertex();

            // do not visit the ignored vertex
            if(currMin.first == initConfig.ignoredVertex_) {
                continue;
            }

            // check if the stopping distance is reached
            if(currMin.second > initConfig.stopDistance_) {
                break;
            }

            // limit the number of hops a path can have
            if(state.numHops(currMin.first) > initConfig.hopNumber_) {
                continue;
            }

            // check the search space size
            if(currSearchSpace > initConfig.searchSpace_) {
                break;
            }

            targetsToVisit.erase(currMin.first);
            currSearchSpace++;

            // finally if all the targets are reached
            if(targetsToVisit.empty()){
#ifdef GRAPH_DEBUG
                LOGG(Logger::INFO) << "[DIJKSTRA NODE] " << currMin.first << Logger::FLUSH;
                LOGG(Logger::INFO) << "[DIJKSTRA] path length " << currMin.second << Logger::FLUSH;
                LOGG(Logger::INFO) << "[DIJKSTRA] vertices visited: " << vertices << Logger::FLUSH;
                LOGG(Logger::INFO) << "[DIJKSTRA]: path" << Logger::FLUSH;
#endif
                // fill in the distances to targets in the init config
                std::unordered_map<VertexId, DistType> targetDistances;
                for(VertexId id : initConfig.targetVertices_) {
                    targetDistances[id] = state.distTo(id);
                }
                result.setDistToTargets(targetDistances);
                result.setSearchSpace(currSearchSpace);
#ifdef GRAPH_DEBUG
                printPath(result.getPath());
#endif
                return currMin.second;
            }

            // get the outgoing edges
            auto outGoingEnd = this->gModel_->getOutgoingIterEnd(currMin.first);
            auto outGoingCur = this->gModel_->getOutgoingIterBegin(currMin.first);

            size_t prevHops = state.numHops(currMin.first);
            for(; outGoingCur != outGoingEnd; ++outGoingCur) {
                if(!state.relaxEdge(currMin, prevHops, outGoingCur))
                    break;
            }
        }

        // fill in the distances anyway
        std::unordered_map<VertexId, DistType> targetDistances;
        for(VertexId id : initConfig.targetVertices_) {
            targetDistances[id] = state.distTo(id);
        }
        result.setDistToTargets(targetDistances);
        result.setSearchSpace(currSearchSpace);

        return std::numeric_limits<DistType>::max();
    }
};

template<class GraphModel>
class Graph : public BaseGraph<GraphModel> {
public:
    typedef typename GraphModel::VertexType VertexType;
    typedef typename VertexType::VertexId VertexId;
    typedef typename GraphModel::EdgeType EdgeType;
    typedef typename EdgeType::EdgeId EdgeId;
    typedef typename GraphModel::DistType DistType;

    typedef typename GraphModel::NearestPointResult NearestPointResult;
    typedef typename GraphModel::SearchResult SearchResult;
    typedef typename GraphModel::AlgorithmInit DijkstraInit;
    typedef typename GraphModel::AlgorithmState DijkstraState;
public:
    // Object pool initializer and destuctor
    class Initializer {
    private:
        std::string filename_;
    public:
        Initializer(const std::string &filename)
            : filename_(filename) {;}

        bool init(Graph<GraphModel> &graph) const {
            return graph.parseGraph(filename_);
        }
    };

    class Destructor {
    public:
        static bool release(Graph<GraphModel> &graph) {
            graph.unloadGraph();
            return true;
        }
    };

    /**
     * @brief Graph
     */
    Graph() : BaseGraph<GraphModel>() {;}

    /**
     * shortest path using Dijkstras algorithm
     * @brief shortestPathAStar
     * @param s
     * @param d
     * @param path
     * @return
     */
    DistType shortestPathDijkstra(const Point &src, const Point &dst, SearchResult &result) const {
#ifdef GRAPH_DEBUG
        size_t vertices = 0;
#endif

        DijkstraInit initConfig;
        if(!this->gModel_->getInitConfig(src, dst, initConfig))
            return -1;

        result = SearchResult(initConfig.getSrcSearchResult(), initConfig.getDstSearchResult());

        DijkstraState state(this->gModel_);
        state.init(initConfig);

#ifdef GRAPH_DEBUG
        DistType oldDistance = 0;
#endif

        while(!state.isDone()) {
#ifdef GRAPH_DEBUG
            vertices++;
#endif

            // find the current closest vertex
            auto currMin = state.getNextVertex();

#ifdef GRAPH_DEBUG
            if(currMin.second < oldDistance) {
                LOGG(Logger::INFO) << "wrong distance" << Logger::FLUSH;
                LOGG(Logger::INFO) << currMin.second << " " << oldDistance << Logger::FLUSH;
            }

            oldDistance = currMin.second;
#endif

            // finally if the end is reached
            if(initConfig.isEndReached(currMin.first)) {
#ifdef GRAPH_DEBUG
                LOGG(Logger::INFO) << "[DIJKSTRA NODE] " << currMin.first << Logger::FLUSH;
                LOGG(Logger::INFO) << "[DIJKSTRA] path length " << currMin.second << Logger::FLUSH;
                LOGG(Logger::INFO) << "[DIJKSTRA] vertices visited: " << vertices << Logger::FLUSH;
                LOGG(Logger::INFO) << "[DIJKSTRA]: path" << Logger::FLUSH;
#endif
                VertexId start = state.getStartPoint(initConfig);
                result.setPath(state.unrollPath(start, currMin.first));
                result.setLength(currMin.second);

                std::vector<Edge::EdgeId> wayIds;
                if(this->findOrigWayIds(result, wayIds)) {
                    result.setOrigIds(wayIds);
                } else {
                    LOGG(Logger::INFO) << "[DIJKSTRA]: couldn't get all original way ids" << Logger::FLUSH;
                }
#ifdef GRAPH_DEBUG
                printPath(result.getPath());
#endif
                return currMin.second;
            }

            // get the outgoing edges
            auto outGoingEnd = this->gModel_->getOutgoingIterEnd(currMin.first);
            auto outGoingCur = this->gModel_->getOutgoingIterBegin(currMin.first);

            // update the shortest distances
            for(; outGoingCur != outGoingEnd; ++outGoingCur) {
                state.relaxEdge(currMin, outGoingCur);
            }
        }

        return -1;
    }

    /**
     * shortest path using A* algorithm
     * @brief shortestPathAStar
     * @param s
     * @param d
     * @param path
     * @return
     */
    template<typename Metric=Edge::DistanceMetric>
    DistType shortestPathAStar(const Point &src, const Point &dst, SearchResult &result) const {
#ifdef GRAPH_DEBUG
        size_t vertices = 0;
#endif
        typedef typename GraphModel::AlgorithmStateAStar AStarState;

        DijkstraInit initConfig;
        if(!this->gModel_->getInitConfig(src, dst, initConfig)) {
            return -1;
        }
        result = SearchResult(initConfig.getSrcSearchResult(), initConfig.getDstSearchResult());

        AStarState state(this->gModel_);
        state.init(initConfig);

#ifdef GRAPH_DEBUG
        DistType oldDistance = 0;
#endif

        while(!state.isDone()) {
#ifdef GRAPH_DEBUG
            vertices++;
#endif

            // find the current closest vertex
            auto currMin = state.getNextVertex();
#ifdef GRAPH_DEBUG
            if(currMin.second < oldDistance) {
                LOGG(Logger::INFO) << "wrong distance" << Logger::FLUSH;
                LOGG(Logger::INFO) << currMin.second << " " << oldDistance << Logger::FLUSH;
            }

            oldDistance = currMin.second;
#endif
            // finally if the end is reached
            if(initConfig.isEndReached(currMin.first)){
#ifdef GRAPH_DEBUG
                LOGG(Logger::INFO) << "[DIJKSTRA NODE] " << currMin.first << Logger::FLUSH;
                LOGG(Logger::INFO) << "[DIJKSTRA] path length " << currMin.second << Logger::FLUSH;
                LOGG(Logger::INFO) << "[DIJKSTRA] vertices visited: " << vertices << Logger::FLUSH;
                LOGG(Logger::INFO) << "[DIJKSTRA]: path" << Logger::FLUSH;
#endif
                VertexId start = state.getStartPoint(initConfig);

                result.setPath(state.unrollPath(start, currMin.first));
                result.setLength(currMin.second);

                std::vector<Edge::EdgeId> wayIds;
                if(this->findOrigWayIds(result.getPath(), wayIds)) {
                    result.setOrigIds(wayIds);
                } else {
                    LOGG(Logger::INFO) << "[ASTAR]: couldn't get all original way ids" << Logger::FLUSH;
                }
#ifdef GRAPH_DEBUG
                printPath(result.getPath());
#endif
                return currMin.second;
            }

            // get the outgoing edges
            auto outGoingEnd = this->gModel_->getOutgoingIterEnd(currMin.first);
            auto outGoingCur = this->gModel_->getOutgoingIterBegin(currMin.first);

            // point of the current min distance vertex
            DistType heuristicOld = state.getHeuristic(currMin.first);

            // update the shortest distances
            for(; outGoingCur != outGoingEnd; ++outGoingCur) {
                state.relaxEdge(currMin, heuristicOld, outGoingCur);
            }
        }

        return -1;
    }

    /**
     * shortest path using bidirectional A* algorithm
     * @brief shortestPathBidirectionalAStar
     * @param s
     * @param d
     * @param path
     * @return
     */
    DistType shortestPathBidirectionalAStar(const Point &src, const Point &dst, SearchResult &result) {
        typedef typename GraphModel::AlgorithmStateAStar AStarState;

        // time inner Dijkstra
        Timer timer;

        DijkstraInit initConfig;
        if(!this->gModel_->getInitConfig(src, dst))
            return -1;

        result = SearchResult(initConfig.getSrcSearchResult(), initConfig.getDstSearchResult());

        AStarState stateF(this->gModel_), stateB(this->gModel_);
        stateF.init(initConfig);

        DijkstraInit revConf = initConfig.reverseConfig();
        stateB.init(revConf, true);

        // a common meeting point
        VertexId commonVertex = VertexType::NullVertexId;

        // last seen shortest paths
        DistType lastSeenForward = std::numeric_limits<DistType>::max(),
                lastSeenBackward = lastSeenForward, shortestSoFar = lastSeenBackward;

#ifdef GRAPH_DEBUG
        LOGG(Logger::INFO) << "[BIDIJKSTRA] Bidirectional vertices: " << s << d << Logger::FLUSH;
#endif

        bool forewardDone = false, backwardDone = false;
        while(!forewardDone || !backwardDone) {
            // forward Dijkstra step
            if(!stateF.isDone()) {
                // current point on the frontier
                auto currMin = stateF.getNextVertex();
                lastSeenForward = currMin.second;

                if(lastSeenForward <= shortestSoFar) {
                    // check if vertex is seen from both ends
                    if(stateB.wasSeen(currMin.first)) {
                        DistType shortestCurrent = stateB.distTo(currMin.first)+currMin.second;
                        if(shortestCurrent < shortestSoFar) {
                            shortestSoFar = shortestCurrent;
                            commonVertex = currMin.first;
                        }
                    }

                    DistType heuristicOld = stateF.getHeuristic(currMin.first);

                    // get the outgoing edges
                    auto outGoingEnd = this->gModel_->getOutgoingIterEnd(currMin.first);
                    auto outGoingCur = this->gModel_->getOutgoingIterBegin(currMin.first);

                    // update the shortest distances
                    for(; outGoingCur != outGoingEnd; ++outGoingCur) {
                        stateF.relaxEdge(currMin, dst, heuristicOld, outGoingCur);
                    }
                } else {
                    forewardDone = true;
                }
            } else {
                forewardDone = true;
            }

            // backward Dijkstra step
            if(!stateB.isDone()) {
                // current point on the frontier
                auto currMin = stateB.getNextVertex();
                lastSeenBackward = currMin.second;

                if(lastSeenBackward <= shortestSoFar) {
                    // check if vertex is seen from both ends
                    if(stateF.wasSeen(currMin.first)) {
                        DistType shortestCurrent = stateF.distTo(currMin.first)+currMin.second;
                        if(shortestCurrent < shortestSoFar) {
                            shortestSoFar = shortestCurrent;
                            commonVertex = currMin.first;
                        }
                    }

                    DistType heuristicOld = stateB.getHeuristic(currMin.first);

                    // get the outgoing edges
                    auto inComingEnd = this->gModel_->getIncomingIterEnd(currMin.first);
                    auto inComingCur = this->gModel_->getIncomingIterBegin(currMin.first);

                    // update the shortest distances
                    for(; inComingCur != inComingEnd; ++inComingCur) {
                        stateB.relaxEdge(currMin, src, heuristicOld, inComingCur);
                    }
                } else {
                    backwardDone = true;
                }
            } else {
                backwardDone = true;
            }
        }

        // stopping criterion
        if(shortestSoFar != std::numeric_limits<DistType>::max()) {
#ifdef GRAPH_DEBUG
            LOGG(Logger::INFO) << "[BIDIJKSTRA] common vertex: " << commonVertex.getId() << Logger::FLUSH;
            LOGG(Logger::INFO) << "[BIDIJKSTRA] common vertices visited: " << vertices << Logger::FLUSH;
            LOGG(Logger::INFO) << "[BIDIJKSTRA] shortest path: " << shortestSoFar << Logger::FLUSH;
            LOGG(Logger::INFO) << "[BIDIJKSTRA PATH]: " << Logger::FLUSH;
#endif
            timer.stop();
            LOGG(Logger::INFO) << "[INNER BIASTAR] " << timer.getElapsedTimeSec() << Logger::FLUSH;

            VertexId start = stateF.getStartPoint(initConfig, commonVertex, true);
            VertexId target = stateB.getStartPoint(revConf, commonVertex, true);

            result.setPath(this->unrollPathBiDir(start, commonVertex, target, stateB, stateF));
            result.setLength(shortestSoFar);

            std::vector<Edge::EdgeId> wayIds;
            if(this->findOrigWayIds(result.getPath(), wayIds)) {
                result.setOrigIds(wayIds);
            } else {
                LOGG(Logger::INFO) << "[BIASTAR]: couldn't get all original way ids" << Logger::FLUSH;
            }

#ifdef GRAPH_DEBUG
            printPath(path);
#endif
            return shortestSoFar;
        } else {
#ifdef GRAPH_DEBUG
            LOGG(Logger::INFO) << "[BIDIJKSTRA] path not found" << Logger::FLUSH;
#endif
        }

        timer.stop();
        LOGG(Logger::INFO) << "[INNER BIASTAR] " << timer.getElapsedTimeSec() << Logger::FLUSH;

        return -1;
    }


    /**
     * shortest path using bidirectional Dijkstra algorithm
     * @brief shortestPathBidirectionalDijkstra
     * @param s
     * @param d
     * @param path
     * @return
     */
    template<typename Metric=Edge::DistanceMetric>
    DistType shortestPathBidirectionalDijkstra(const Point &src, const Point &dst, SearchResult &result) {
        // time inner Dijkstra
        Timer timer;

        DijkstraInit initConfig;
        if(!this->gModel_->getInitConfig(src, dst, initConfig))
            return -1;
        result = SearchResult(initConfig.getSrcSearchResult(), initConfig.getDstSearchResult());

        DijkstraState stateF(this->gModel_), stateB(this->gModel_, true);
        stateF.init(initConfig);

        DijkstraInit revConf = initConfig.reverseConfig();
        stateB.init(revConf);

        // a common meeting point
        VertexId commonVertex = VertexType::NullVertexId;

        // last seen shortest paths
        DistType lastSeenForward = std::numeric_limits<DistType>::max(),
                 lastSeenBackward = lastSeenForward, shortestSoFar = lastSeenBackward;

    #ifdef GRAPH_DEBUG
        LOGG(Logger::INFO) << "[BIDIJKSTRA] Bidirectional vertices: " << s << d << Logger::FLUSH;
    #endif

        bool forewardDone = false, backwardDone = false;
        while(!forewardDone || !backwardDone) {
            // forward Dijkstra step
            if(!stateF.isDone()) {
                // current point on the frontier
                auto currMin = stateF.getNextVertex();
                lastSeenForward = currMin.second;

                if(lastSeenForward <= shortestSoFar) {
                    // check if vertex is seen from both ends
                    if(stateB.wasSeen(currMin.first)) {
                        DistType shortestCurrent = stateB.distTo(currMin.first)+currMin.second;
                        if(shortestCurrent < shortestSoFar) {
                            shortestSoFar = shortestCurrent;
                            commonVertex = currMin.first;
                        }
                    }

                    // get the outgoing edges
                    auto outGoingEnd = this->gModel_->getOutgoingIterEnd(currMin.first);
                    auto outGoingCur = this->gModel_->getOutgoingIterBegin(currMin.first);

                    // update the shortest distances
                    for(; outGoingCur != outGoingEnd; ++outGoingCur) {
                        stateF.relaxEdge(currMin, outGoingCur);
                    }
                } else {
                    forewardDone = true;
                }
            } else {
                forewardDone = true;
            }

            // backward Dijkstra step
            if(!stateB.isDone()) {
                // current point on the frontier
                auto currMin = stateB.getNextVertex();
                lastSeenBackward = currMin.second;

                if(lastSeenBackward <= shortestSoFar) {
                    // check if vertex is seen from both ends
                    if(stateF.wasSeen(currMin.first)) {
                        DistType shortestCurrent = stateF.distTo(currMin.first)+currMin.second;
                        if(shortestCurrent < shortestSoFar) {
                            shortestSoFar = shortestCurrent;
                            commonVertex = currMin.first;
                        }
                    }

                    // get the outgoing edges
                    auto inComingEnd = this->gModel_->getIncomingIterEnd(currMin.first);
                    auto inComingCur = this->gModel_->getIncomingIterBegin(currMin.first);

                    // update the shortest distances
                    for(; inComingCur != inComingEnd; ++inComingCur) {
                        stateB.relaxEdge(currMin, inComingCur);
                    }
                } else {
                    backwardDone = true;
                }
            } else {
                backwardDone = true;
            }
        }

        // stopping criterion
        if(shortestSoFar != std::numeric_limits<DistType>::max()) {
    #ifdef GRAPH_DEBUG
            LOGG(Logger::INFO) << "[BIDIJKSTRA] common vertex: " << commonVertex.getId() << Logger::FLUSH;
            LOGG(Logger::INFO) << "[BIDIJKSTRA] common vertices visited: " << vertices << Logger::FLUSH;
            LOGG(Logger::INFO) << "[BIDIJKSTRA] shortest path: " << shortestSoFar << Logger::FLUSH;
            LOGG(Logger::INFO) << "[BIDIJKSTRA PATH]: " << Logger::FLUSH;
    #endif
            timer.stop();
            LOGG(Logger::INFO) << "[INNER BIDIJKSTRA] " << timer.getElapsedTimeSec() << Logger::FLUSH;

            VertexId start = stateF.getStartPoint(initConfig, commonVertex);
            VertexId target = stateB.getStartPoint(revConf, commonVertex);

            result.setPath(this->unrollPathBiDir(start, commonVertex, target, stateB, stateF));
            result.setLength(shortestSoFar);

            std::vector<Edge::EdgeId> wayIds;
            if(this->findOrigWayIds(result, wayIds)) {
                result.setOrigIds(wayIds);
            } else {
                LOGG(Logger::INFO) << "[BIDIJKSTRA]: couldn't get all original way ids" << Logger::FLUSH;
            }

    #ifdef GRAPH_DEBUG
            printPath(path);
    #endif

            return shortestSoFar;
        } else {
    #ifdef GRAPH_DEBUG
            LOGG(Logger::INFO) << "[BIDIJKSTRA] path not found" << Logger::FLUSH;
    #endif
        }

        timer.stop();
        LOGG(Logger::INFO) << "[INNER BIDIJKSTRA] " << timer.getElapsedTimeSec() << Logger::FLUSH;

        return -1;
    }
};
