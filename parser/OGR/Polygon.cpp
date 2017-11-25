#include <cmath>
#include <cassert>
#include "OGR/Polygon.h"
#include <UrbanLabs/Sdk/Utils/GPolyEncode.h>

using namespace std;

/**
 * @brief Polygon::addRing
 * @param ringPts
 */
void Polygon::addRing(const vector<Point> &ringPts) {
    points_.push_back(ringPts);
}
/**
 * @brief Polygon::addPoint
 * @param pt
 */
void Polygon::addPoint(const Point &/*pt*/) {
    assert(false);
}
/**
 * @brief Polygon::getPoints
 */
vector<vector<Point> > Polygon::getPoints() const {
    return points_;
}
/**
 * @brief Polygon::getArea
 * @return
 */
double Polygon::getArea() const {
    double area = 0.0;
    for (int j = 0; (int)j < points_.size(); ++j) {
        double total = 0;
        int N = points_[j].size();
        for(int i = 0; i < N; i++) {
            total += (points_[j][i].lat()+points_[j][(i+1)%N].lon())*(points_[j][i].lat()-points_[j][(i+1)%N].lat());
        }
        area = max(area, (abs(total)/2)*METERS_IN_LAT*METERS_IN_LON);
        assert(area >= 0 && area < numeric_limits<double>::max());
    }
    return area;
}
/**
 * @brief LineString::getBoundingBox
 * @return
 */
vector<Point> Polygon::getBoundingBox(bool inMercator) const {
    if(points_.size() > 0) {
        if(inMercator) {
            Point::CoordType maxLat = Point::lat2y_m(points_[0][0].lat()),
                    maxLon = Point::lon2x_m(points_[0][0].lon()),
                    minLat = maxLat, minLon = maxLon;
            for (int i=1; i < (int)points_[0].size();++i) {
                maxLat = max(Point::lat2y_m(points_[0][i].lat()), maxLat);
                maxLon = max(Point::lon2x_m(points_[0][i].lon()), maxLon);
                minLat = min(Point::lat2y_m(points_[0][i].lat()), minLat);
                minLon = min(Point::lon2x_m(points_[0][i].lon()), minLon);
            }
            Point nw(minLat,minLon);
            Point se(maxLat,maxLon);

            return {{nw, se}};
        }
        Point::CoordType maxLat = points_[0][0].lat(), maxLon = points_[0][0].lon(),
                minLat = maxLat, minLon = maxLon;
        for (int i=1; i < (int)points_[0].size();++i) {
            maxLat = max(points_[0][i].lat(), maxLat);
            maxLon = max(points_[0][i].lon(), maxLon);
            minLat = min(points_[0][i].lat(), minLat);
            minLon = min(points_[0][i].lon(), minLon);
        }
        Point nw(minLat,minLon);
        Point se(maxLat,maxLon);

        return {{nw, se}};
    }
    return {{0.0, 0.0}};
}

string Polygon::encode() const {
    return GPolyEncoder::encodePoints(points_);
}
