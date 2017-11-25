#pragma once

#include <UrbanLabs/Sdk/GraphCore/Point.h>

#include <geos/geom/Geometry.h>
#include <geos/geom/LineString.h>
#include <geos/geom/LinearRing.h>
#include <geos/geom/Polygon.h>
#include <geos/geom/MultiPolygon.h>

#include <set>
#include <vector>

// linestring simplification
void simplifyLine(std::vector<Point> &pts, double eps);
void simplifyPreserveTopologyLine(std::vector<Point> &pts, double eps);
// polygon simplification
void simplifyPreserveTopologyPolygon(std::vector<Point> &shell, double eps);
void simplifyPreserveTopologyPolygon(std::vector<Point> &shell, std::vector<std::vector<Point> > &holes, double eps);

std::vector<std::vector<Point> > splitLine(const std::vector<Point> &line, size_t skip);
void fixPolygon(const std::vector<int8_t> &found, std::vector<Point> &pts);

// orient polygon clockwise and the holes counterclockwise
void forceWindingOrder(std::vector<Point> &nodes);
// build polygons with holes from segments
void buildPolygons(const std::vector<std::vector<Point> > &parts, std::vector<std::vector<std::vector<Point> > > &rings, double eps = 0);

namespace tools {

std::unique_ptr<geos::geom::LineString> ogrLineToGeos(const std::vector<Point>& pts);
std::unique_ptr<geos::geom::LinearRing> ogrRingToGeos(const std::vector<Point>& pts);
std::unique_ptr<geos::geom::Polygon> ogrPolygonToGeos(const std::vector<Point>& shell, const std::vector<std::vector<Point> >& rings);
std::unique_ptr<geos::geom::MultiPolygon> ogrMultiPolygonToGeos(const std::vector<std::vector<std::vector<Point> > > & polys);
std::string geosToWkt(const geos::geom::Geometry* geom);
std::unique_ptr<geos::geom::Geometry> wkbToGeos(const std::string &wkb);
std::vector<std::vector<Point> > readWktLineStrings(const std::vector<std::string> &wkts);
std::string linestringToWKT(const std::vector<Point>& line);
std::string polygonToWKT(const std::vector<Point>& shell, const std::vector<std::vector<Point> >& holes);
std::string polygonToWKT(const std::vector<Point>& shell);
std::string multiPolygonToWkt(const std::vector<std::vector<std::vector<Point> > >& pts);
void intersect(const std::vector<Point> &bbox, const std::vector<std::string> &wkbs, std::vector<std::vector<std::vector<Point> > > &rings);
}
