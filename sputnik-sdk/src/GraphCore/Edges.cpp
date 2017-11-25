// Edges.cpp
//

#include <UrbanLabs/Sdk/GraphCore/Edges.h>

using namespace std;

const Edge::EdgeId Edge::NullEdgeId = -1;
const Edge::EdgeDist Edge::MaxDist = numeric_limits<Edge::EdgeDist>::max();
const Edge::EdgeTime Edge::MaxTime = numeric_limits<Edge::EdgeTime>::max();

/**
 * @brief getCost
 * @return
 */
template<>
typename Edge::TimeMetric::Metric Edge::getCost<Edge::TimeMetric>() const {
    return time_;
}

namespace std {
    // specialize hashing function for edges
//    template<>
//    class hash<Edge> {
//    public:
//        size_t operator()(const Edge &e) const {
//            return hash<Edge::EdgeId>()(e.getId());
//        }
//    };
}
