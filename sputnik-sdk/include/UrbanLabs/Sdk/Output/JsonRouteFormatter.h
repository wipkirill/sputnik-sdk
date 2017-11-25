#pragma once

#include <UrbanLabs/Sdk/Platform/Stdafx.h>
#include <UrbanLabs/Sdk/GraphCore/Edges.h>
#include <UrbanLabs/Sdk/GraphCore/Vertices.h>
#include <UrbanLabs/Sdk/Output/JsonXMLFormatter.h>

class JsonRouteFormatter {
public:
    typedef JSONFormatterNode Node;
    typedef std::vector <Node> Nodes;
public:
    Node root_;
    template <typename T>
    std::string toString(T t);
    friend std::ostream &operator << (std::ostream &oss, const JsonRouteFormatter &js) {
        oss << js.root_;
        return oss;
    }
    template <typename T, typename V>
    Nodes textAndValue(T text, V value);
    void addRoot(const Point &startPoint, const Point &endPoint);
    void addRoute(const Point &startPoint, const Point &endPoint, JsonRouteFormatter::Nodes &legs, const Edge::EdgeDist length);
    Node getLeg(const std::vector <std::vector<Point> > &multiLines, const std::vector<Edge::EdgeId> &origWayIds,
                const Point &startPoint, const Point &endPoint,
                const Edge::EdgeDist length, const Edge::EdgeDist duration);
    Node addStep(const Edge::EdgeId id, const Point &startPoint, const Point &endPoint,
                 const std::vector <Point> &multiLine,
                 const Point::PointDistType length, const Point::PointDistType duration,
                 const std::string &instruction = "");
};
/**
 *
 */
template <typename T>
std::string JsonRouteFormatter::toString(T t) {
    std::stringstream ss;
    ss << std::setprecision(9);
    ss << t;
    return ss.str();
}
/**
 *
 */
template <typename T, typename V>
JsonRouteFormatter::Nodes JsonRouteFormatter::textAndValue(T text, V value) {
    return {Node("text", toString(text)), Node("value", value)};
}
