// Point.cpp
//

#include <memory>
#include <cassert>
#include <UrbanLabs/Sdk/GraphCore/Point.h>
#include <UrbanLabs/Sdk/Utils/GPolyEncode.h>

using namespace std;

#define Sdeg2rad(d) ((d*M_PI)/180)
#define Srad2deg(d) ((d*180)/M_PI)
#define earth_radius 6378137

const Point::CoordType Point::NullCoord = 361;
const Point::PointDistType Point::NullDist = -1;

/**
 * @brief Point::Point
 */
Point::Point() : lat_(0), lon_(0) {;}
/**
 * @brief Point::Point
 * @param lat
 * @param lon
 */
Point::Point(CoordType la, CoordType lo)
    : lat_(la), lon_(lo) {
    ;
}
/**
 * @brief Point::setLat
 * @param lat
 */
void Point::setLat(CoordType la) {
    lat_ = la;
}
/**
 * @brief Point::setLon
 * @param lon
 */
void Point::setLon(CoordType lo) {
    lon_ = lo;
}
/**
 * @brief Point::lon
 * @return
 */
Point::CoordType Point::lon() const {
    return lon_;
}
/**
 * @brief Point::lat
 * @return
 */
Point::CoordType Point::lat() const {
    return lat_;
}
/**
 * @brief Point::setLatLon
 * @param lat
 * @param lon
 */
void Point::setLatLon(CoordType la, CoordType lo) {
    lat_ = la;
    lon_ = lo;
}
/**
 * @brief Point::operator <<
 */
ostream &operator << (ostream &os, const Point &pt) {
    os << "[" << pt.lat() << ", " << pt.lon() << "]";
    return os;
}
/**
 * @brief Point::operator ==
 * @param pt
 * @return
 */
bool Point::operator == (const Point &pt) const {
    return lat_ == pt.lat() && lon_ == pt.lon();
}
/**
 * @brief Point::serialize
 * @param os
 * @return
 */
ostream &Point::serialize(ostream &os) const {
    os << lat() << " " << lon();
    return os;
}
/**
 * @brief Point::getArea
 */
double Point::getArea() const {
    return 0;
}

Point::PointDistType pointDistance(const Point &from, const Point &to) {
#if USE_MSSE_SINE_COSINE
    double latitudeArc  = (from.lat() - to.lat()) * DEG_TO_RAD_2;
    double longitudeArc = (from.lon() - to.lon()) * DEG_TO_RAD_2;
    v4sf x = _mm_set_ps(to.lat()*DEG_TO_RAD, from.lat()*DEG_TO_RAD, longitudeArc, latitudeArc);

    v4sf y, z;
    sincos_ps(x, &y, &z);
    float arr_sin[4] __attribute__((aligned(16)));
    _mm_store_ps(arr_sin, y);

    float arr_cos[4] __attribute__((aligned(16)));
    _mm_store_ps(arr_cos, z);

    double tmp = arr_cos[2]*arr_cos[3];
    double latitudeH = arr_sin[0]; //sin(latitudeArc);
    double lontitudeH = arr_sin[1]; //sin(longitudeArc);
#elif USE_LUT_SINE_COSINE
    double latitudeArc  = (from.lat() - to.lat());
    double longitudeArc = (from.lon() - to.lon());

    double tmp = getCosLUT(from.lat())*getCosLUT(to.lat());
    double latitudeH = getSinLUT(latitudeArc/2);
    double lontitudeH = getSinLUT(longitudeArc/2);
#else
    double latitudeArc  = (from.lat() - to.lat()) * DEG_TO_RAD;
    double longitudeArc = (from.lon() - to.lon()) * DEG_TO_RAD;
    double latitudeH = sin(latitudeArc * 0.5);
    double lontitudeH = sin(longitudeArc * 0.5);
    double tmp = cos(from.lat()*DEG_TO_RAD) * cos(to.lat()*DEG_TO_RAD);
#endif

    latitudeH *= latitudeH;
    lontitudeH *= lontitudeH;

    return EARTH_RADIUS_IN_METERS*asin(sqrt(latitudeH + tmp*lontitudeH));
}

Point::PointDistType manhattanDistance(const Point &from, const Point &to) {
    return fabs(from.lat()-to.lat())+fabs(from.lon()-to.lon());
}

size_t closestPoint(const Point &ref, const vector <Point> &points) {
    size_t closest = 0;
    Point::PointDistType dist = numeric_limits<Point::PointDistType>::max();

    for(size_t i = 0; i < points.size(); i++) {
        Point::PointDistType currDist = manhattanDistance(ref, points[i]);
        if(currDist < dist) {
            dist = currDist;
            closest = i;
        }
    }

    return closest;
}

vector<Point> computeBoundingBox(const Point &p1, const Point &p2) {
    Point::CoordType maxLat, maxLon, minLat, minLon;

    maxLat = max(p1.lat(), p2.lat());
    minLat = min(p1.lat(), p2.lat());
    maxLon = max(p1.lon(), p2.lon());
    minLon = min(p1.lon(), p2.lon());

    Point nw(maxLat,minLon);
    Point se(minLat,maxLon);

    return {{nw, se}};
}

vector<Point> Point::getBoundingBox(bool inMercator) const {
    if(inMercator) {
        Point pt(lat2y_m(lat_),lon2x_m(lon_));
        return {pt, pt};
    }
    return {Point(lat_,lon_), Point(lat_, lon_)};
}

Point::PointDistType getLength(const vector <Point> &line) {
    Point::PointDistType length = 0;
    for(size_t i = 0; i < line.size(); i++) {
        if(i != 0) {
            length += pointDistance(line[i], line[i-1]);
        }
    }
    return length;
}

Point centerOfMass(const vector<Point> &pts) {
    if(pts.size() == 0)
        return {0, 0};

    Point::CoordType lat = 0, lon = 0;
    for(const Point &pt : pts) {
        lat += pt.lat();
        lon += pt.lon();
    }
    return Point(lat/pts.size(), lon/pts.size());
}

Point Point::toMercator(const Point &pt) {
    double lat = lat2y_m(pt.lat());
    double lon = lon2x_m(pt.lon());
    return {lat, lon};
}

double Point::lat2y_m(double lt) {
    return earth_radius*log(tan(M_PI/4+ Sdeg2rad(lt)/2));
}

double Point::lon2x_m(double ln) {
    return Sdeg2rad(ln)*earth_radius;
}

string Point::encode() const {
    return GPolyEncoder::encodePoints({{lat_, lon_}});
}
