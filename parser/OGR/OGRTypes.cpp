#include <cstring>
#include "OGR/OGRTypes.h"
#include <iostream>
#include <assert.h>
using namespace std;

//--------------------------------------------------------------------------------
// wkbSize
//--------------------------------------------------------------------------------
template<>
int wkbSize<Point>(const Point &/*obj*/) {
    return 1 + 2 * sizeof(Point::CoordType);
}

template<>
int wkbSize<Polygon>(const Polygon &obj) {
    int nSize = 1 + 2 * sizeof(int);
    vector<vector<Point> > pts = obj.getPoints();
    for(int i = 0; i<(int)pts.size(); ++i) {
        nSize += sizeof(int) + 2 * sizeof(Point::CoordType) * pts[i].size();
    }
    return nSize;
}

template<>
int wkbSize<LineString>(const LineString &obj) {
    vector<Point> pts = obj.getPoints();
    return 1 + 2 * sizeof(int) + 2 * sizeof(Point::CoordType) * pts.size();
}
//--------------------------------------------------------------------------------
// getGeometryType
//--------------------------------------------------------------------------------
template<>
OGRwkbGeometryType getGeometryType<Point>() {
	return wkbPoint;
}

template<>
OGRwkbGeometryType getGeometryType<Polygon>() {
	return wkbPolygon;
}

template<>
OGRwkbGeometryType getGeometryType<LineString>() {
	return wkbLineString;
}
//--------------------------------------------------------------------------------
// exportToWkb
//--------------------------------------------------------------------------------
template<>
void exportToWkb<Point>(const Point &obj, OGRwkbByteOrder eByteOrder, unsigned char * pabyData, int offset, bool inMercator) {
	pabyData[offset] = (unsigned char) eByteOrder;

    GUInt32 nGType = getGeometryType<Point>();

    if( eByteOrder == wkbNDR )
        nGType = CPL_LSBWORD32(nGType);
    else
        nGType = CPL_MSBWORD32(nGType);
    Point pt = obj;
    if(inMercator)
        pt = Point::toMercator(pt);
    Point::CoordType lat = pt.lat(), lon = pt.lon();
    memcpy(pabyData + offset + 1, &nGType, sizeof(GUInt32));
    memcpy(pabyData + offset + sizeof(GUInt32)+1, &lon, sizeof(Point::CoordType));
    memcpy(pabyData + offset + 5 + sizeof(Point::CoordType), &lat, sizeof(Point::CoordType));
}

template<>
void exportToWkb<Polygon>(const Polygon &obj, OGRwkbByteOrder eByteOrder, unsigned char * pabyData, int offset, bool inMercator) {
	//Set the byte order.
    pabyData[offset] = (unsigned char) eByteOrder;

    //Set the geometry feature type.
    GUInt32 nGType = getGeometryType<Polygon>();
    if(eByteOrder == wkbNDR)
        nGType = CPL_LSBWORD32(nGType);
    else
        nGType = CPL_MSBWORD32(nGType);

    memcpy(pabyData+offset+1, &nGType, 4);

    vector<vector<Point> > pts = obj.getPoints();
    int nRingCount = pts.size();
    memcpy(pabyData+offset+5, &nRingCount, 4);
    int nOffset = 9+offset;

    size_t ptLen = sizeof(Point::CoordType);
    for(int i = 0; i < pts.size(); ++i) {
        // copy number of points
        int nPointCount = (int)pts[i].size();
        memcpy(pabyData + nOffset, &nPointCount, 4 );
        nOffset += 4;

        int ptOffset = 0;
        //Serialize each of the rings
        for(int j = 0; j < (int)pts[i].size(); ++j) {
            Point pt = pts[i][j];
            if(inMercator)
                pt = Point::toMercator(pt);
            Point::CoordType x = pt.lon();
            Point::CoordType y = pt.lat();
            memcpy(pabyData+nOffset+2*j*ptLen, &x, ptLen);
            memcpy(pabyData+nOffset+2*j*ptLen+ptLen, &y, ptLen);
            ptOffset += 2 * ptLen;
        }
        nOffset += ptOffset;
    }
}

template<>
void exportToWkb<LineString>(const LineString &obj, OGRwkbByteOrder eByteOrder, unsigned char * pabyData, int offset, bool inMercator) {
	pabyData[offset] = (unsigned char) eByteOrder;
    GUInt32 nGType = getGeometryType<LineString>();

    if(eByteOrder == wkbNDR)
        nGType = CPL_LSBWORD32(nGType);
    else
        nGType = CPL_MSBWORD32(nGType);

    memcpy(pabyData + offset + 1, &nGType, 4 );

    // Copy in the data count
    vector<Point> pts = obj.getPoints();
    int nPointCount = (int)pts.size();
    memcpy(pabyData + offset + 5, &nPointCount, 4 );
    
    // Copy in the raw data
    for (int i = 0; i < (int)pts.size();++i) {
        Point pt = pts[i];
        if(inMercator)
            pt = Point::toMercator(pt);
        Point::CoordType x = pt.lon();
        Point::CoordType y = pt.lat();
        memcpy(pabyData+offset+9+2*i*sizeof(Point::CoordType), &x, sizeof(Point::CoordType));
        memcpy(pabyData+offset+9+2*i*sizeof(Point::CoordType)+sizeof(Point::CoordType), &y, sizeof(Point::CoordType));
    }
}
