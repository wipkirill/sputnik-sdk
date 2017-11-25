#pragma once

#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/GraphCore/Vertices.h>
#include <UrbanLabs/Sdk/Utils/Logger.h>

// a typical edge
class Edge {
public:
    typedef int EdgeDist;
    typedef int EdgeTime;
    typedef int64_t EdgeId;
    typedef unsigned char EdgeType;
    typedef Vertex::VertexId VertexId;
    typedef std::pair<Vertex::VertexId, Vertex::VertexId> EdgeKey;
    static const EdgeDist MaxDist;
    static const EdgeTime MaxTime;
public:
    static const EdgeId NullEdgeId;
public:
    // types of metrics used
    class TimeMetric {
    public:
        typedef EdgeTime Metric;
    };
    class DistanceMetric {
    public:
        typedef EdgeDist Metric;
    };
public:
    /**
     * @brief The EdgeTypeEnum enum
     */
    enum EdgeTypeEnum {
        // carefull adding new types, check parser where edge types
        // are combined when generating turn restriction edges
        GEOMETRY_STORED = 1,
        ONE_WAY = 2,
        CREATED_ON_PREPROCESSING = 4,
        TURN_RESTRICTED = 8,
        FINAL = 16
    };
protected:
    Vertex next_;
    VertexId via_;
    EdgeType type_;
    EdgeDist dist_;
    EdgeTime time_;
    EdgeId origId_;
public:
    /**
     * @brief Edge
     */
    Edge() : next_(Vertex::NullVertexId), via_(Vertex::NullVertexId),
        type_(0), dist_(0), time_(0), origId_(Edge::NullEdgeId)
    {
        ;
    }
    /**
     * @brief Edge
     * @param next
     * @param cost
     * @param time
     * @param via
     * @param type
     */
    Edge(Vertex next, EdgeDist dist, EdgeTime time, VertexId via, EdgeType type, EdgeId origId)
        : next_(next), via_(via), type_(type), dist_(dist), time_(time), origId_(origId) {
        ;
    }

    // getter methods

    /**
     * @brief getNext
     * @return
     */
    Vertex getNext() const {
        return next_;
    }

    /**
     * @brief getNextId
     * @return
     */
    VertexId getNextId() const {
        return next_.getId();
    }

    /**
     * @brief getVia
     * @return
     */
    VertexId getVia() const {
        return via_;
    }

    /**
     * @brief getType
     * @return
     */
    EdgeType getType() const {
        return type_;
    }

    /**
     * @brief getCost
     * @return
     */
    template<typename Metric = DistanceMetric>
    typename Metric::Metric getCost() const {
        return dist_;
    }

    /**
     * @brief getOrigId
     * @return
     */
    Edge::EdgeId getOrigId() const {
        return origId_;
    }

    /**
     * @brief setNextId
     * @param id
     */
    void setNextId(VertexId id) {
        next_.setId(id);
    }

    /**
     * @brief setVia
     * @param via
     */
    void setVia(VertexId via) {
        via_ = via;
    }

    /**
     * @brief setCost
     * @param cost
     */
    void setTime(EdgeTime time) {
        time_ = time;
    }

    /**
     * @brief setCost
     * @param cost
     */
    void setDist(EdgeDist dist) {
        dist_ = dist;
    }

    /**
     * @brief serialize
     * @param oss
     * @return
     */
    std::ostream& serialize(std::ostream& oss) const {
        oss << next_.getId() << " " << via_ << " " << dist_ << " " << time_ << " " << type_ << " " << origId_ <<  std::endl;
        return oss;
    }
    /**
     * @brief serializeToFile
     * @param file
     */
    void serializeToFile(FILE* file) {
        std::string sep = StringConsts::SEPARATOR;
        std::string vIdpSpec  = type_spec<VertexId>(),
               distpSpec = type_spec<EdgeDist>(),
               timepSpec = type_spec<EdgeTime>(),
               typepSpec = type_spec<EdgeType>(),
               origIdpSpec = type_spec<EdgeId>();

        std::string printFormat = vIdpSpec+sep+vIdpSpec+sep+
               distpSpec+sep+timepSpec+sep+typepSpec+sep+origIdpSpec+"\n";
        fprintf(file, printFormat.c_str(), next_.getId(), via_, dist_, time_, type_, origId_);
    }
    /**
     * @brief deserialize
     * @param iss
     * @return
     */
    template <typename T>
    T& deserialize(T& iss) {
        EdgeType type;
        EdgeDist dist;
        EdgeTime time;
        VertexId via, nextId;
        EdgeId origId;

        iss >> nextId >> via >> dist >> time >> type >> origId;
        via_ = via, dist_ = dist, time_ = time, type_ = type, origId_ = origId;
        next_.setId(nextId);
        return iss;
    }
};

// forward edges
class EdgeForw : public Edge {
public:
    /**
     * @brief EdgeForw
     */
    EdgeForw() {
        ;
    }

    /**
     * @brief EdgeForw
     * @param next
     * @param cost
     * @param via
     * @param type
     */
    EdgeForw(Vertex next, EdgeDist dist, EdgeTime time, VertexId via,
             EdgeType type, EdgeId origId = Edge::NullEdgeId)
        : Edge(next, dist, time, via, type, origId) {
        ;
    }
};

// backward edges
class EdgeBack : public Edge {
public:
    /**
     * @brief EdgeBack
     */
    EdgeBack() {
        ;
    }

    /**
     * @brief EdgeBack
     * @param next
     * @param cost
     * @param via
     * @param type
     */
    EdgeBack(Vertex next, EdgeDist dist, EdgeTime time, VertexId via,
             EdgeType type, EdgeId origId = Edge::NullEdgeId)
        : Edge(next, dist, time, via, type, origId) {
        ;
    }
};

namespace std {
    // specialize hashing function for edge keys
    template<>
    class hash<Edge::EdgeKey> {
    public:
        size_t operator()(const Edge::EdgeKey &k) const {
            return std::hash<Vertex::VertexId>()(k.first) ^ std::hash<Vertex::VertexId>()(k.second);
        }
    };
}

/**
 * doesn't depend on arguments the order
 * @brief edgeKey
 * @param ver1
 * @param ver2
 * @return
 */
inline Edge::EdgeKey edgeKey(const Vertex &ver1, const Vertex &ver2) {
    if (ver1.getId() > ver2.getId())
        return Edge::EdgeKey(ver2.getId(), ver1.getId());
    return Edge::EdgeKey (ver1.getId(), ver2.getId());
}

/**
 * depends on arguments order
 * @brief edgeKeyFixed
 * @param ver1
 * @param ver2
 * @return
 */
inline Edge::EdgeKey edgeKeyFixed(const Vertex &ver1, const Vertex &ver2) {
    return Edge::EdgeKey (ver1.getId(), ver2.getId());
}
