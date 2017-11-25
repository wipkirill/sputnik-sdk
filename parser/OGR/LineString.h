#pragma once

#include <UrbanLabs/Sdk/GraphCore/Point.h>

/**
 * @brief The LineString class
 */
class LineString {
private:
    std::vector<Point> points_;
public:
    void addPoint(const Point &pt);
    std::vector<Point> getPoints() const;
    std::vector<Point> getBoundingBox(bool inMercator = false) const;
    double getArea() const;
    void clearPoints();
    std::string encode() const;
};

