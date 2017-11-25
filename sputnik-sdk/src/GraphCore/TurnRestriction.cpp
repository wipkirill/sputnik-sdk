#include <UrbanLabs/Sdk/GraphCore/TurnRestriction.h>
//------------------------------------------------------------------------------
// Turn restrictions
//------------------------------------------------------------------------------
/**
 * @brief TurnRestriction
 * @param from
 * @param to
 * @param via
 * @param type
 */
TurnRestriction::TurnRestriction(const Edge::EdgeId &from, const Edge::EdgeId &to,
                                 const Vertex::VertexId &via)
    : via_(via), from_(from), to_(to) {;}
/**
 * @brief TurnRestriction
 * @param from
 * @param to
 * @param via
 * @param type
 */
TurnRestriction::TurnRestriction(const Edge::EdgeId &from, const Edge::EdgeId &to,
                                 const Vertex::VertexId &via, const std::string &type)
    : type_(type), via_(via), from_(from), to_(to) {;}

/**
 * @brief getType
 * @return
 */
std::string TurnRestriction::getType() const {
    return type_;
}

/**
 * @brief getVia
 * @return
 */
Vertex::VertexId TurnRestriction::getVia() const {
    return via_;
}

/**
 * @brief getFrom
 * @return
 */
Edge::EdgeId TurnRestriction::getFrom() const {
    return from_;
}

/**
 * @brief getTo
 * @return
 */
Edge::EdgeId TurnRestriction::getTo() const {
    return to_;
}
