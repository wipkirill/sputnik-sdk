#pragma once

#include <UrbanLabs/Sdk/GraphCore/Edges.h>
#include <UrbanLabs/Sdk/GraphCore/Vertices.h>

/**
 * @brief The TurnRestriction class
 */
class TurnRestriction {
private:
    std::string type_;
    Vertex::VertexId via_;
    Edge::EdgeId from_, to_;
public:
    TurnRestriction(const Edge::EdgeId &from, const Edge::EdgeId &to,
                    const Vertex::VertexId &via);
    TurnRestriction(const Edge::EdgeId &from, const Edge::EdgeId &to,
                    const Vertex::VertexId &via, const std::string &type);
    std::string getType() const;
    Vertex::VertexId getVia() const;
    Edge::EdgeId getFrom() const;
    Edge::EdgeId getTo() const;
};

namespace std {
    // specialize hash function for turn restrictions
    template<>
    class hash<TurnRestriction> {
    public:
        size_t operator()(const TurnRestriction &t) const {
            return hash<Vertex::VertexId>()(t.getVia()) ^ hash<Edge::EdgeId>()(t.getFrom()) ^ hash<Edge::EdgeId>()(t.getTo());
        }
    };
}
