#pragma once

#include <vector>
#include <limits>
#include <complex>
#include <ostream>

#include <UrbanLabs/Sdk/Platform/Stdafx.h>

enum StoredType : char {
    COMPRESSED = 1,
    POINT = 2,
    LINE = 4,
    POLYGON = 6,
    MULTIPOLYGON = 8
};

static const double EARTH_RADIUS_IN_METERS = 2.0*6372797.560856;
static const double DEG_TO_RAD = 0.017453292519943295769236907684886;
static const double DEG_TO_RAD_2 = 0.017453292519943295769236907684886*0.5;
static const double METERS_IN_LAT = 110574.61;
static const double METERS_IN_LON = 111302.62;

/**
 * @brief The Point class
 */
class Point {
public:
    typedef double CoordType;
    typedef double PointDistType;
public:
    static const CoordType NullCoord;
    static const PointDistType NullDist;
private:
    CoordType lat_;
    CoordType lon_;
public:
    Point();
    Point(CoordType lat, CoordType lon);
    void setLat(CoordType lat);
    void setLon(CoordType lon);
    bool operator == (const Point &pt) const;
    void setLatLon(CoordType lat, CoordType lon);
    CoordType lon() const;
    CoordType lat() const;
    template <typename T>
    friend T &operator >> (T &iss, Point &pt) {
        CoordType lat, lon;
        iss >> lat >> lon;

        pt.setLatLon(lat, lon);

        return iss;
    }
    friend std::ostream &operator << (std::ostream &os, const Point &pt);
    std::ostream &serialize(std::ostream &os) const;
    friend Point::PointDistType pointDistance(const Point &from, const Point &to);
    friend Point::PointDistType manhattanDistance(const Point &from, const Point &to);
    friend std::vector<Point> computeBoundingBox(const Point &p1, const Point &p2);
    friend size_t closestPoint(const Point &ref, const std::vector<Point> &points);
    std::vector<Point> getBoundingBox(bool inMercator = false) const;
    friend Point::PointDistType getLength(const std::vector <Point> &line);
    friend Point centerOfMass(const std::vector<Point> &pts);
    static Point toMercator(const Point &pt);
    static double lat2y_m(double lt);
    static double lon2x_m(double ln);
    double getArea() const;
    std::string encode() const;
};

