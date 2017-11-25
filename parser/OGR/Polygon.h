#pragma once

#include <vector>
#include "OGR/LineString.h"
#include <UrbanLabs/Sdk/GraphCore/Point.h>

/**
 * @brief The Polygon class
 */
class Polygon : public LineString {
protected:
    std::vector<std::vector<Point> > points_;
public:
    void addPoint(const Point &pt);
    void addRing(const std::vector<Point> &ringPts);
    std::vector<Point> getBoundingBox(bool inMercator = false) const;
    std::vector<std::vector<Point> > getPoints() const;
    double getArea() const;
    std::string encode() const;
};
