// JsonRouteFormatter.cpp
//

#include <Output/JsonRouteFormatter.h>

using namespace std;

/**
 * @brief JsonRouteFormatter::addRoot
 * @param startPoint
 * @param endPoint
 */
void JsonRouteFormatter::addRoot(const Point &startPoint, const Point &endPoint) {
    vector<Point> bbox = computeBoundingBox(startPoint, endPoint);

    Node boxEl("boundingBox");
    boxEl.add(Nodes {Node("nw",bbox[0]), Node("se", bbox[1])});
    root_.add(Nodes {boxEl});
}
/**
 * @brief JsonRouteFormatter::addRoute
 * @param startPoint
 * @param endPoint
 * @param legs
 * @param length
 */
void JsonRouteFormatter::addRoute(const Point &startPoint, const Point &endPoint, JsonRouteFormatter::Nodes &legs, const Edge::EdgeDist length) {

    Node startEl("start_location", startPoint);
    Node endEl("end_location", endPoint);
    Node distance("distance");
    distance.add(textAndValue(length, length));
    Node duration("duration");
    duration.add(textAndValue(length, length));

    // add steps
    Node legsEl("legs", legs);
    Node routeEl("");
    routeEl.add(Nodes {distance, duration, startEl, endEl, legsEl});
    root_.add(Nodes {Node("routes", Nodes{routeEl})});
}
/**
 * @brief JsonRouteFormatter::getLeg
 * @param multiLines
 * @param origWayIds
 * @param startPoint
 * @param endPoint
 * @param length
 * @param duration
 * @return
 */
JsonRouteFormatter::Node JsonRouteFormatter::getLeg(const vector <vector<Point> > &multiLines,
                                                    const vector<Edge::EdgeId> &origWayIds,
                                                    const Point &startPoint, const Point &endPoint,
                                                    const Edge::EdgeDist length, const Edge::EdgeDist duration) {
    Node leg;
    Node distanceEl("distance");
    distanceEl.add(textAndValue(length, length));
    Node durationEl("duration");
    durationEl.add(textAndValue(duration, duration));
    Node startEl("start_location", startPoint);
    Node endEl("end_location", endPoint);

    // add steps
    Nodes allSteps;
    for(int i = 0; i < multiLines.size(); i++) {
        int first = 0, last = multiLines[i].size()-1;
        allSteps.push_back(addStep(origWayIds[i], multiLines[i][first], multiLines[i][last], multiLines[i],
                                   getLength(multiLines[i]), getLength(multiLines[i])));
    }

    Node steps("steps", allSteps);
    leg.add(Nodes {distanceEl, durationEl, startEl, endEl, steps});

    return leg;
}
/**
 * @brief JsonRouteFormatter::addStep
 * @param id
 * @param startPoint
 * @param endPoint
 * @param multiLine
 * @param length
 * @param duration
 * @param instruction
 * @return
 */
JsonRouteFormatter::Node JsonRouteFormatter::addStep(const Edge::EdgeId id,
                                                     const Point &startPoint, const Point &endPoint,
                                                     const vector <Point> &multiLine, const Point::PointDistType length,
                                                     const Point::PointDistType duration, const string &instruction) {
    Node stepEl;
    Node wayId("wayid");
    wayId.add(textAndValue(id, id));
    Node distanceEl("distance");
    distanceEl.add(textAndValue(length, length));
    Node durationEl("duration");
    durationEl.add(textAndValue(duration, duration));
    Node startEl("start_location", startPoint);
    Node endEl("end_location", endPoint);

    Node polylineEl("polyline", multiLine);
    Node instructionEl("instruction", instruction);

    stepEl.add(Nodes {wayId, durationEl, distanceEl, startEl, endEl, polylineEl, instructionEl});
    return stepEl;
}
