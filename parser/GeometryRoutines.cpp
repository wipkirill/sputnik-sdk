#include "GeometryRoutines.h"

#include <UrbanLabs/Sdk/Utils/Logger.h>

#include <geos/geom/prep/PreparedGeometry.h>
#include <geos/geom/prep/PreparedGeometryFactory.h>

#include <geos/geom/CoordinateArraySequence.h>
#include <geos/geom/CoordinateSequenceFactory.h>
#include <geos/geom/GeometryFactory.h>
#include <geos/geom/Point.h>
#include <geos/geom/Geometry.h>
#include <geos/geom/Coordinate.h>
#include <geos/io/WKTWriter.h>
#include <geos/io/WKTReader.h>
#include <geos/io/WKBReader.h>
#include <geos/index/strtree/STRtree.h>
#include <geos/index/ItemVisitor.h>
#include <geos/opLinemerge.h>

#include <geos/simplify/TopologyPreservingSimplifier.h>
#include <geos/simplify/DouglasPeuckerSimplifier.h>
#include <geos/simplify/DouglasPeuckerLineSimplifier.h>

#include <geos/util/IllegalArgumentException.h>

#include <geos/algorithm/CGAlgorithms.h>

#include <vector>
#include <type_traits>
#include <algorithm>
#include <memory>
#include <sstream>
#include <queue>
#include <map>

using geos::geom::Polygon;
using geos::geom::MultiPolygon;
using geos::geom::Envelope;
using geos::geom::LineString;
using geos::geom::LinearRing;
using geos::geom::CoordinateSequence;
using geos::geom::CoordinateArraySequence;
using geos::geom::Coordinate;
using geos::geom::GeometryFactory;
using geos::geom::Geometry;
using geos::geom::prep::PreparedGeometryFactory;
using geos::geom::prep::PreparedGeometry;

typedef std::unique_ptr<Geometry> GeomPtr;

class FactoryStorage {
private:
    static GeometryFactory* geomFactory;

public:
    FactoryStorage()
    {
        geomFactory = nullptr;
    }
    static const GeometryFactory* getGeomFactory()
    {
        if (geomFactory)
            return geomFactory;
        else
            return geomFactory = new GeometryFactory();
    }
};

GeometryFactory* FactoryStorage::geomFactory = nullptr;
static FactoryStorage factory = FactoryStorage();

namespace impl {
std::string lineToWKT(const std::vector<Point>& shell)
{
    std::string res = "(";
    for (size_t i = 0; i < shell.size(); ++i) {
        res.append(std::to_string(shell[i].lon()) + " " + std::to_string(shell[i].lat()));
        if (i + 1 != shell.size())
            res.append(",");
    }
    res.append(")");
    return res;
}

bool isClosed(const std::vector<Point>& pts)
{
    const static double thres = 1e-12;
    if (pts.size() >= 4) {
        return fabs(pts[0].lat() - pts[pts.size() - 1].lat()) < thres && fabs(pts[0].lon() - pts[pts.size() - 1].lon()) < thres;
    }
    return false;
}

std::vector<Point> geosLineStringToOgr(const LineString* lineStr)
{
    std::vector<Point> result;
    for (size_t i = 0; i < lineStr->getNumPoints(); ++i) {
        Point pt(lineStr->getCoordinateN(i).y, lineStr->getCoordinateN(i).x);
        result.push_back(pt);
    }
    return result;
}

std::vector<Point> geosRingToOgr(const LinearRing* ring)
{
    std::vector<Point> result;
    for (size_t i = 0; i < ring->getNumPoints(); ++i) {
        Point pt(ring->getCoordinateN(i).y, ring->getCoordinateN(i).x);
        result.push_back(pt);
    }
    return result;
}

struct PreparedGeomDeleter {
    void operator()(PreparedGeometry* geometry)
    {
        return PreparedGeometryFactory::destroy(geometry);
    }
};

struct PolygonWrapper {
    explicit PolygonWrapper(const Geometry* geometry)
        : polygon(const_cast<PreparedGeometry*>(PreparedGeometryFactory::prepare(geometry)))
        , isInner(false)
        , area(geometry->getArea())
    {
        ;
    }

    ~PolygonWrapper()
    {
        polygon.reset(nullptr);
    }

    const Geometry* getGeometry() const
    {
        if (polygon)
            return &polygon->getGeometry();
        return nullptr;
    }

    const Envelope* getEnvelopeInternal() const
    {
        return getGeometry()->getEnvelopeInternal();
    }

    double getArea() const
    {
        return area;
    }

    bool covers(const PolygonWrapper* wrapper) const
    {
        return getGeometry()->covers(wrapper->getGeometry());
    }

    std::unique_ptr<PreparedGeometry, PreparedGeomDeleter> polygon;
    mutable bool isInner;
    double area;
};

class PolygonVisitor : public geos::index::ItemVisitor {
public:
    PolygonVisitor(const PolygonWrapper* wrapper)
        : child_(wrapper)
        , parent_(nullptr)
        , smallestArea_(0)
    {
        ;
    }

    virtual void visitItem(void* what)
    {
        PolygonWrapper* parent = reinterpret_cast<PolygonWrapper*>(what);
        if (smallestArea_ <= 0 || parent->getArea() < smallestArea_) {
            if (parent != child_ && parent->covers(child_)) {
                child_->isInner = !parent->isInner;
                smallestArea_ = parent->getArea();
                parent_ = parent;
            }
        }
    }

    const Geometry* getParent()
    {
        if (parent_) {
            return parent_->getGeometry();
        }
        return nullptr;
    }

    const PolygonWrapper* getWrapper() const
    {
        return parent_;
    }

private:
    const PolygonWrapper* child_;
    PolygonWrapper* parent_;
    double smallestArea_;
};

int distinctPoints(const Geometry* geom)
{
    std::unique_ptr<geos::geom::CoordinateSequence> seq(geom->getCoordinates());
    std::vector<geos::geom::Coordinate> coords;
    for(size_t i = 0; i < seq->size(); ++i) {
        coords.emplace_back(seq->getAt(i));
    }
    std::sort(coords.begin(), coords.end());
    coords.resize(std::unique(coords.begin(), coords.end())-coords.begin());
    return coords.size();
}

// this is a trick to split self intersecting line strings into non-self intersecting
// http://tsusiatsoftware.net/jts/files/jts_secrets_foss4g2007.pdf
std::unique_ptr<Geometry> removeSelfIntersections(std::unique_ptr<Geometry> geom)
{
    const GeometryFactory* gf = factory.getGeomFactory();
    std::unique_ptr<geos::geom::Point> point(gf->createPoint(*geom->getCoordinate()));
    // this will free up the old geometry and replace with the new one
    // that doesn't have self intersections
    int before = distinctPoints(geom.get());
    std::unique_ptr<geos::geom::Geometry> split(
        geom->Union(static_cast<geos::geom::Geometry*>(point.get()))
    );
    int after = distinctPoints(split.get());
    if (!geom->isEmpty() && before != after) {
        throw std::runtime_error("Self intersection at a non vertex is not allowed");
    }
    return split;
}
/**
 * @brief mergeLineStrings
 */
std::unique_ptr<std::vector<LineString*> > mergeLineStrings(std::unique_ptr<std::vector<Geometry*> > lines)
{
    const GeometryFactory* gf = factory.getGeomFactory();

    // do not touch the polygonal lines, but remove from others self intersections
    // by breaking them at the points of self intersection
    size_t initSize = lines->size();
    std::vector<LineString*> skipped;
    for (size_t i = 0; lines && i < initSize; ++i) {
        if ((*lines)[i]) {
            assert((*lines)[i]->getGeometryTypeId() == geos::geom::GEOS_LINESTRING);
            LineString* lineStr = dynamic_cast<LineString*>((*lines)[i]);

            if (!lineStr->isSimple()) {
                // this will free up the old geometry and replace with the new one
                // that doesn't have self intersections, note that ownership to
                // line[i] goes into removeSelfIntersections
                std::unique_ptr<geos::geom::Geometry> split = removeSelfIntersections(
                    std::unique_ptr<geos::geom::Geometry>((*lines)[i]));
                for (size_t j = 0; j < split->getNumGeometries(); j++) {
                    if (j == 0) {
                        (*lines)[i] = split->getGeometryN(j)->clone();
                    } else {
                        lines->push_back(split->getGeometryN(j)->clone());
                    }
                    assert(split->getGeometryN(j)->getGeometryTypeId() == geos::geom::GEOS_LINESTRING);
                }
            } else {
                if (lineStr->isClosed()) {
                    skipped.push_back(lineStr);
                    (*lines)[i] = nullptr;
                }
            }
        }
    }

    lines->erase(std::remove_if(lines->begin(), lines->end(), [](const Geometry* geom) {
        return geom == nullptr;
    }), lines->end());

    // again remove self intersections, as after creation of
    // multiline some new ones can appear, see the test 
    // "2 touching rings on single points"
    geos::operation::linemerge::LineMerger merger;
    GeomPtr multiLine(gf->createMultiLineString(lines.release()));
    multiLine = removeSelfIntersections(std::move(multiLine));
    merger.add(multiLine.get());

    std::unique_ptr<std::vector<LineString*> > merged(merger.getMergedLineStrings());

    // finally put back the valid polygons
    for (size_t i = 0; i < skipped.size(); ++i) {
        merged->push_back(skipped[i]);
    }

    return merged;
}

GeomPtr cloneExteriorRing(const Geometry* geom)
{
    return GeomPtr(dynamic_cast<const Polygon*>(geom)->getExteriorRing()->clone());
}
} // namespace impl

using namespace impl;

void simplifyLine(std::vector<Point>& pts, double eps)
{
    GeomPtr line(tools::ogrLineToGeos(pts));
    if (line.get() && !line->isValid()) {
        LOGG(Logger::ERROR) << "Input geometry invalid. Removed it from dataset." << Logger::FLUSH;
        pts.clear();
        return;
    }

    GeomPtr geom = geos::simplify::DouglasPeuckerSimplifier::simplify(line.get(), eps);

    if (geom.get() != nullptr || !geom->isValid() || geom->getNumGeometries() != 1) {
        LOGG(Logger::ERROR) << "Invalid geometry on simplification" << Logger::FLUSH;
        return;
    }

    if (geom->getGeometryN(0)->getGeometryTypeId() != geos::geom::GEOS_LINESTRING) {
        LOGG(Logger::ERROR) << "Wrong type after geometry simplification" << Logger::FLUSH;
        return;
    }

    std::unique_ptr<LineString> simple(dynamic_cast<LineString*>(geom.release()));
    pts = geosLineStringToOgr(simple.get());
}

void simplifyPreserveTopologyLine(std::vector<Point>& pts, double eps)
{
    GeomPtr line(tools::ogrLineToGeos(pts));
    if (line.get() && !line->isValid()) {
        LOGG(Logger::ERROR) << "Input geometry invalid. Removed it from dataset." << Logger::FLUSH;
        pts.clear();
        return;
    }

    GeomPtr geom = geos::simplify::TopologyPreservingSimplifier::simplify(line.get(), eps);

    if (!geom->isValid() || geom->getNumGeometries() != 1) {
        LOGG(Logger::ERROR) << "Invalid geometry on simplification" << Logger::FLUSH;
        return;
    }

    if (geom->getGeometryN(0)->getGeometryTypeId() != geos::geom::GEOS_LINESTRING) {
        LOGG(Logger::ERROR) << "Wrong type after geometry simplification" << Logger::FLUSH;
        return;
    }

    std::unique_ptr<LineString> simple(dynamic_cast<LineString*>(geom.release()));
    pts = geosLineStringToOgr(simple.get());
}

void simplifyPreserveTopologyPolygon(std::vector<Point>& shell, double eps)
{
    std::vector<std::vector<Point> > holes;
    simplifyPreserveTopologyPolygon(shell, holes, eps);
}

void simplifyPreserveTopologyPolygon(std::vector<Point>& shell, std::vector<std::vector<Point> >& holes, double eps)
{
    GeomPtr polygon;

    try {
        polygon = std::move(tools::ogrPolygonToGeos(shell, holes));
        if (polygon.get() && !polygon->isValid()) {
            LOGG(Logger::ERROR) << "Input geometry not valid. Removed it from dataset." << Logger::FLUSH;
            LOGG(Logger::ERROR) << tools::polygonToWKT(shell, holes) << Logger::FLUSH;
            shell.clear();
            holes.clear();
            return;
        }
    }
    catch (geos::util::IllegalArgumentException& e) {
        LOGG(Logger::ERROR) << e.what() << Logger::FLUSH;
        LOGG(Logger::ERROR) << tools::polygonToWKT(shell, holes) << Logger::FLUSH;
        shell.clear();
        holes.clear();
        return;
    }
    catch (const std::runtime_error& e) {
        LOGG(Logger::ERROR) << e.what() << Logger::FLUSH;
        shell.clear();
        holes.clear();
        return;
    }

    GeomPtr geom = geos::simplify::TopologyPreservingSimplifier::simplify(polygon.get(), eps);

    if (geom->getNumGeometries() != 1) {
        LOGG(Logger::ERROR) << "Too many geometries in output" << Logger::FLUSH;
        return;
    }

    if (!geom->isValid() || !geom->isSimple()) {
        LOGG(Logger::ERROR) << "Invalid geometry on simplification" << Logger::FLUSH;
        return;
    }

    if (geom->getGeometryN(0)->getGeometryTypeId() != geos::geom::GEOS_POLYGON) {
        LOGG(Logger::ERROR) << "Wrong type after geometry simplification" << Logger::FLUSH;
        return;
    }

    const Polygon* simple = dynamic_cast<const Polygon*>(geom->getGeometryN(0));
    const LinearRing* outerRing = dynamic_cast<const LinearRing*>(simple->getExteriorRing());
    std::vector<Point> outer = geosRingToOgr(outerRing);

    std::vector<std::vector<Point> > inner(simple->getNumInteriorRing());
    for (size_t i = 0; i < simple->getNumInteriorRing(); ++i) {
        const LinearRing* hole = dynamic_cast<const LinearRing*>(simple->getInteriorRingN(i));
        inner[i] = geosRingToOgr(hole);
    }

    shell = outer;
    holes = inner;
}
/**
 * @brief splitLine
 * @param line the line to be split
 * @param skip maximum number of points in a split segment 
 */
std::vector<std::vector<Point> > splitLine(const std::vector<Point>& line, size_t skip)
{
    std::vector<Point> curr;
    curr.reserve(std::min(skip, line.size()));

    std::vector<std::vector<Point> > segs;
    for (size_t i = 0; i < line.size(); ++i) {
        curr.push_back(line[i]);

        if (curr.size() >= skip || i + 1 == line.size()) {
            segs.push_back(curr);
            curr.clear();
        }
    }

    return segs;
}
/**
 * @brief fixPolygon will snap points that are not found to the closest ones that are present
 * @param polygon a polygon to fix
 */
void fixPolygon(const std::vector<int8_t>& foundConst, std::vector<Point>& pts)
{
    // TODO: this does not work correctly, some points are mapped
    //       to points too far away
    // fix broken polygons on the border of map
    // map missing points to the nearest ones that are present
    std::vector<int8_t> found = foundConst;
    for (size_t i = 0; i < found.size(); ++i) {
        if (i > 0 && !found[i] && found[i - 1]) {
            pts[i] = pts[i - 1];
            found[i] = 1;
        }
    }

    for (int i = (int)found.size() - 1; i >= 0; i--) {
        if (i + 1 < (int)found.size() && !found[i] && found[i + 1]) {
            pts[i] = pts[i + 1];
            found[i] = 1;
        }
    }
}
/**
 * @brief forceWindingOrder
 * @param nodes
 * https://github.com/openstreetmap/osm2pgsql/blob/master/geometry-builder.cpp
 * Fn geometry_builder::get_wkt_simple
 */
void forceWindingOrder(std::vector<Point>& nodes)
{
    const GeometryFactory* gf = factory.getGeomFactory();
    std::unique_ptr<CoordinateSequence> coords(gf->getCoordinateSequenceFactory()->create((size_t)0, (size_t)2));

    try {
        for (size_t i = 0; i < nodes.size(); ++i) {
            coords->add(Coordinate(nodes[i].lon(), nodes[i].lat()), 0);
        }

        GeomPtr geom;
        if ((coords->getSize() >= 4) && (coords->getAt(coords->getSize() - 1).equals2D(coords->getAt(0)))) {
            std::unique_ptr<LinearRing> shell(gf->createLinearRing(coords.release()));
            geom = GeomPtr(gf->createPolygon(shell.release(), new std::vector<Geometry*>));
            if (!geom->isValid()) {
                throw std::runtime_error("Broken polygon.");
            }
            // fix direction of ring
            // put the smallest point as first
            geom->normalize();
        } else {
            throw std::runtime_error("Degenerate polygon.");
        }
        std::unique_ptr<CoordinateSequence> normCoords(geom->getCoordinates());
        nodes.clear();
        for (size_t i = 0; i < normCoords->size(); ++i) {
            nodes.push_back(Point(normCoords->getAt(i).y, normCoords->getAt(i).x));
        }
    }
    catch (const std::bad_alloc&) {
        LOGG(Logger::ERROR) << "Out of memory." << Logger::FLUSH;
    }
    catch (const std::runtime_error& e) {
        LOGG(Logger::ERROR) << e.what() << Logger::FLUSH;
    }
    catch (...) {
        LOGG(Logger::ERROR) << "Exception on forceWindingOrder" << Logger::FLUSH;
    }
}
/**
 * @brief buildPolygons
 */
void buildPolygons(const std::vector<std::vector<Point> >& parts, std::vector<std::vector<std::vector<Point> > >& rings, double eps)
{
    try {
        const GeometryFactory* gf = factory.getGeomFactory();
        std::unique_ptr<std::vector<Geometry*> > lines(new std::vector<Geometry*>);

        for (size_t i = 0; i < parts.size(); ++i) {
            if (parts[i].size() > 1) {
                lines->push_back(static_cast<Geometry*>(tools::ogrLineToGeos(parts[i]).release()));
            }
        }

        // merge polylines to polygons
        std::vector<std::unique_ptr<PolygonWrapper> > wrap;
        std::unique_ptr<std::vector<LineString*> > merged(mergeLineStrings(std::move(lines)));

        for (size_t i = 0; i < merged->size(); ++i) {
            bool ok = false;
            std::unique_ptr<LineString> polyLine((*merged)[i]);
            if (polyLine->getNumPoints() >= 4 && polyLine->isClosed() && polyLine->isValid()) {
                GeomPtr polygon(static_cast<Geometry*>(gf->createPolygon(gf->createLinearRing(polyLine->getCoordinates()), 0)));
                if (polygon->getArea() > 0 && polygon->isSimple()) {
                    wrap.emplace_back(new PolygonWrapper(polygon.release()));
                    ok = true;
                }
            }
            if (!ok)
                throw std::runtime_error("Could not build polygon from given parts");
        }

        // build hierarchy of polygons
        geos::index::strtree::STRtree rtree;
        for (size_t i = 0; i < wrap.size(); ++i) {
            rtree.insert(wrap[i]->getEnvelopeInternal(),
                         reinterpret_cast<void*>(wrap[i].get()));
        }

        std::sort(wrap.begin(), wrap.end(),
                  [](const std::unique_ptr<PolygonWrapper>& p1,
                     const std::unique_ptr<PolygonWrapper>& p2) {
                return p1->getArea() > p2->getArea();
        });

        // find direct parents of polygons
        // (those that a polygon is contained in completely)
        std::map<const Geometry*, std::vector<const Geometry*> > childs_;
        for (size_t i = 0; i < wrap.size(); ++i) {
            PolygonVisitor visitor(wrap[i].get());
            rtree.query(wrap[i]->getEnvelopeInternal(), visitor);

            const Geometry* parent = visitor.getParent();
            if (parent != nullptr) {
                assert(wrap[i]->isInner || visitor.getWrapper()->isInner);
                if (childs_.count(parent)) {
                    childs_[parent].emplace_back(wrap[i]->getGeometry());
                } else {
                    childs_[parent] = { wrap[i]->getGeometry() };
                }
            } else {
                assert(!wrap[i]->isInner);
            }
        }

        for (size_t i = 0; i < wrap.size(); ++i) {
            if (!wrap[i]->isInner) {
                // build complete polygon with holes and normalize it
                std::unique_ptr<std::vector<Geometry*> > inner(new std::vector<Geometry*>());
                for (const Geometry* child : childs_[wrap[i]->getGeometry()]) {
                    inner->push_back(cloneExteriorRing(child).release());
                }

                std::unique_ptr<LinearRing> ring(
                    dynamic_cast<LinearRing*>(cloneExteriorRing(wrap[i]->getGeometry()).release()));
                std::unique_ptr<Polygon> built(gf->createPolygon(ring.release(), inner.release()));
                if(built->getArea() < 0) {
                    throw std::runtime_error("Constructed polygon has negative area: "+tools::geosToWkt(built.get()));
                }

                built->normalize();
                // simplify
                GeomPtr geom = geos::simplify::TopologyPreservingSimplifier::simplify(built.get(), eps);
                // convert to ogr representation of polygons
                const LinearRing* outerRing = dynamic_cast<const LinearRing*>(built->getExteriorRing());
                std::vector<std::vector<Point> > out = { std::move(geosRingToOgr(outerRing)) };
                for (size_t j = 0; j < built->getNumInteriorRing(); ++j) {
                    const LinearRing* hole = dynamic_cast<const LinearRing*>(built->getInteriorRingN(j));
                    out.emplace_back(std::move(geosRingToOgr(hole)));
                }

                rings.emplace_back(out);
            }
        }

        // TODO:
        // 1. Cascaded polygon union?
        // http://geos.osgeo.org/doxygen/classgeos_1_1operation_1_1geounion_1_1CascadedPolygonUnion.html
        // take union of all the holes and of all the outer rings?
    }
    catch (const geos::util::IllegalArgumentException& e) {
        LOGG(Logger::ERROR) << "Illegal argument in buildPolygons: " << e.what() << Logger::FLUSH;
        rings.clear();
    }
    catch (const std::exception& e) {
        LOGG(Logger::ERROR) << e.what() << Logger::FLUSH;
        rings.clear();
    }
    catch (...) {
        LOGG(Logger::ERROR) << "Caught unknown exception in buildPolygons" << Logger::FLUSH;
        rings.clear();
    }
}

namespace tools {
std::unique_ptr<LineString> ogrLineToGeos(const std::vector<Point>& pts)
{
    CoordinateSequence::AutoPtr seq(new CoordinateArraySequence(pts.size(), 2));
    for (size_t i = 0; i < pts.size(); ++i) {
        Coordinate crd(pts[i].lon(), pts[i].lat());
        seq->setAt(crd, i);
    }
    std::unique_ptr<CoordinateSequence> fixed(geos::geom::CoordinateSequence::removeRepeatedPoints(seq.get()));
    return std::unique_ptr<LineString>(factory.getGeomFactory()->createLineString(fixed.release()));
}

std::unique_ptr<LinearRing> ogrRingToGeos(const std::vector<Point>& pts)
{
    if (!isClosed(pts)) {
        throw std::runtime_error("Ring is not closed: " + tools::polygonToWKT(pts));
    }
    CoordinateSequence::AutoPtr seq(new CoordinateArraySequence(pts.size(), 2));
    for (size_t i = 0; i < pts.size(); ++i) {
        Coordinate crd(pts[i].lon(), pts[i].lat());
        seq->setAt(crd, i);
    }

    std::unique_ptr<CoordinateSequence> fixed(geos::geom::CoordinateSequence::removeRepeatedPoints(seq.get()));
    std::unique_ptr<LinearRing> ring(factory.getGeomFactory()->createLinearRing(fixed.release()));
    return ring;
}

std::unique_ptr<Polygon> ogrPolygonToGeos(const std::vector<Point>& shell, const std::vector<std::vector<Point> >& rings)
{
    std::unique_ptr<LinearRing> outer(ogrRingToGeos(shell));
    std::unique_ptr<std::vector<Geometry*> > inner(new std::vector<Geometry*>());
    for (size_t i = 0; i < rings.size(); ++i) {
        inner->push_back(static_cast<Geometry*>(ogrRingToGeos(rings[i]).release()));
    }

    // polygon takes ownership of inner and outer
    // so no memory is leaked
    return std::unique_ptr<Polygon>(factory.getGeomFactory()->createPolygon(outer.release(), inner.release()));
}

std::unique_ptr<MultiPolygon> ogrMultiPolygonToGeos(const std::vector<std::vector<std::vector<Point> > >& polys)
{
    std::unique_ptr<std::vector<Geometry*> > gPolys(new std::vector<Geometry*>());
    for (size_t k = 0; k < polys.size(); k++) {
        if (polys[k].size() > 0) {
            std::unique_ptr<LinearRing> outer(ogrRingToGeos(polys[k][0]));
            std::unique_ptr<std::vector<Geometry*> > inner(new std::vector<Geometry*>());
            for (size_t i = 1; i < polys[k].size(); ++i) {
                inner->push_back(static_cast<Geometry*>(ogrRingToGeos(polys[k][i]).release()));
            }

            gPolys->push_back(static_cast<Geometry*>(factory.getGeomFactory()->createPolygon(outer.release(), inner.release())));
        }
    }

    return std::unique_ptr<MultiPolygon>(factory.getGeomFactory()->createMultiPolygon(gPolys.release()));
}

std::string polygonToWKT(const std::vector<Point>& shell)
{
    std::vector<std::vector<Point> > holes = {};
    return polygonToWKT(shell, holes);
}

// plot directly with
// http://arthur-e.github.io/Wicket/sandbox-gmaps3.html
std::string linestringToWKT(const std::vector<Point>& line)
{
    std::string res = "LINESTRING" + impl::lineToWKT(line);
    return res;
}

std::string polygonToWKT(const std::vector<Point>& shell, const std::vector<std::vector<Point> >& holes)
{
    std::string res = "POLYGON(";
    res.append(impl::lineToWKT(shell));
    if (!impl::isClosed(shell))
        throw std::runtime_error("The outer shell is not a valid polygon");
    for (size_t i = 0; i < holes.size(); ++i) {
        res.append(",");
        if (!impl::isClosed(holes[i]))
            throw std::runtime_error("The inner hole is not a valid polygon");
        res.append(impl::lineToWKT(holes[i]));
    }
    res.append(")");
    return res;
}

std::string multiPolygonToWkt(const std::vector<std::vector<std::vector<Point> > >& pts)
{
    std::string res = "MULTIPOLYGON(";
    for (size_t k = 0; k < pts.size(); k++) {
        res.append("(");
        res.append(impl::lineToWKT(pts[k][0]));
        for (size_t i = 1; i < pts[k].size(); ++i) {
            res.append(",");
            res.append(impl::lineToWKT(pts[k][i]));
        }
        res.append(")");
        if (k + 1 < pts.size())
            res.append(",");
    }
    res.append(")");
    return res;
}

std::vector<std::vector<Point> > readWktLineStrings(const std::vector<std::string>& wkts)
{
    geos::io::WKTReader reader;
    std::vector<std::vector<Point> > result;
    for (auto s : wkts) {
        Geometry* geom = reader.read(s);
        std::unique_ptr<LineString> line(dynamic_cast<LineString*>(geom));
        std::unique_ptr<CoordinateSequence> coords(line->getCoordinates());
        std::vector<Point> nodes;
        for (size_t i = 0; i < coords->size(); ++i) {
            nodes.push_back(Point(coords->getAt(i).y, coords->getAt(i).x));
        }
        result.push_back(nodes);
    }

    return result;
}

std::string geosToWkt(const Geometry* geom)
{
    geos::io::WKTWriter writer;
    writer.setTrim(true);
    return writer.write(geom);
}

std::unique_ptr<Geometry> wkbToGeos(const std::string &wkb) 
{
    geos::io::WKBReader wkbReader(*(factory.getGeomFactory()));
    std::stringstream s;
    s << wkb; 
    return std::unique_ptr<Geometry>(wkbReader.readHEX(s));
}

void intersect(const std::vector<Point> &bbox, const std::vector<std::string> &wkbs, std::vector<std::vector<std::vector<Point> > > &rings) 
{   
    std::vector<std::unique_ptr<Geometry>> geoms;
    for(auto wkb : wkbs) 
        geoms.emplace_back(wkbToGeos(wkb));
    std::unique_ptr<Polygon> mainBox = ogrPolygonToGeos(bbox, {});

    std::unique_ptr<std::vector<Geometry*> > newgeoms(new std::vector<Geometry *>);
    for (size_t i = 0; i < geoms.size(); i++) {
        Geometry *g1 = geoms[i].get();
        try {
            Geometry *g3 = mainBox->intersection(g1);
            newgeoms->push_back(g3);
        }
        // Collection are illegal as intersection argument
        catch (const geos::util::IllegalArgumentException& ill) {
	       LOGG(Logger::ERROR) << "Illegal argument in buildPolygons: " << ill.what() << Logger::FLUSH;            
        }
        catch (const std::exception& exc) {
	       LOGG(Logger::ERROR) << "Illegal argument in buildPolygons: " << exc.what() << Logger::FLUSH;
        }
    }
    
    for(size_t i = 0; i < newgeoms->size(); ++i) {
        Geometry *geom = newgeoms->at(i);
        const Polygon* simple = dynamic_cast<const Polygon*>(geom->getGeometryN(0));
        const LinearRing* outerRing = dynamic_cast<const LinearRing*>(simple->getExteriorRing());
        std::vector<std::vector<Point> > out = { std::move(geosRingToOgr(outerRing)) };

        std::vector<std::vector<Point> > inner(simple->getNumInteriorRing());
        for (size_t i = 0; i < simple->getNumInteriorRing(); ++i) {
            const LinearRing* hole = dynamic_cast<const LinearRing*>(simple->getInteriorRingN(i));
            out.emplace_back(std::move(geosRingToOgr(hole)));
        }
        rings.emplace_back(out);
    }
}
}
