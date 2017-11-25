#include "OGR/OGR.h"
#include "OGR/LineString.h"
#include <UrbanLabs/Sdk/Utils/GPolyEncode.h>

using namespace std;

/**
 * @brief LineString::addPoint
 * @param pt
 */
void LineString::addPoint(const Point &pt) {
    points_.push_back(pt);
}
/**
 * @brief LineString::clearPoints
 */
void LineString::clearPoints() {
    points_.clear();
}
/**
 * @brief LineString::getPoints
 */
vector<Point> LineString::getPoints() const {
    return points_;
}
/**
 * @brief LineString::getArea
 * @return
 */
double LineString::getArea() const {
    return 0.0;
}
/**
 * @brief LineString::getBoundingBox
 * @return
 */
vector<Point> LineString::getBoundingBox(bool inMercator) const {
    // return bbox in Mercator(Google)
    if(inMercator) {
        Point::CoordType maxLat = Point::lat2y_m(points_[0].lat()), minLat = maxLat;
        Point::CoordType maxLon = Point::lon2x_m(points_[0].lon()), minLon = maxLon;

        for (size_t i=1; i < points_.size();++i) {
            maxLat = max(Point::lat2y_m(points_[i].lat()), maxLat);
            maxLon = max(Point::lon2x_m(points_[i].lon()), maxLon);
            minLat = min(Point::lat2y_m(points_[i].lat()), minLat);
            minLon = min(Point::lon2x_m(points_[i].lon()), minLon);
        }

        Point nw(minLat,minLon);
        Point se(maxLat,maxLon);

        return {{nw, se}};
    } else {
        Point::CoordType maxLat = points_[0].lat(), minLat = maxLat;
        Point::CoordType maxLon = points_[0].lon(), minLon = maxLon;

        for (size_t i=1; i < points_.size();++i) {
            maxLat = max(points_[i].lat(), maxLat);
            maxLon = max(points_[i].lon(), maxLon);
            minLat = min(points_[i].lat(), minLat);
            minLon = min(points_[i].lon(), minLon);
        }

        Point nw(minLat,minLon);
        Point se(maxLat,maxLon);

        return {{nw, se}};
    }
}

string LineString::encode() const {
    return GPolyEncoder::encodePoints(points_);
}
