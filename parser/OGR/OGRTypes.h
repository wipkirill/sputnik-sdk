#pragma once

#include "OGR/OGR.h"
#include "OGR/Polygon.h"
#include "OGR/LineString.h"
#include <UrbanLabs/Sdk/GraphCore/Point.h>

//--------------------------------------------------------------------------------
// wkbSize
//--------------------------------------------------------------------------------
template<typename T>
int wkbSize(const T &obj);

template<>
int wkbSize<Point>(const Point &obj);

template<>
int wkbSize<Polygon>(const Polygon &obj);

template<>
int wkbSize<LineString>(const LineString &obj);

//--------------------------------------------------------------------------------
// getGeometryType
//--------------------------------------------------------------------------------
template<typename T>
OGRwkbGeometryType getGeometryType();

template<>
OGRwkbGeometryType getGeometryType<Point>();

template<>
OGRwkbGeometryType getGeometryType<Polygon>();

template<>
OGRwkbGeometryType getGeometryType<LineString>();
//--------------------------------------------------------------------------------
// exportToWkb
//--------------------------------------------------------------------------------
template<typename T>
void exportToWkb(const T &obj, OGRwkbByteOrder eByteOrder, unsigned char * pabyData, int offset = 0, bool inMercator = true);

template<>
void exportToWkb<Point>(const Point &obj, OGRwkbByteOrder eByteOrder, unsigned char * pabyData, int offset, bool inMercator);

template<>
void exportToWkb<Polygon>(const Polygon &obj, OGRwkbByteOrder eByteOrder, unsigned char * pabyData, int offset, bool inMercator);

template<>
void exportToWkb<LineString>(const LineString &obj, OGRwkbByteOrder eByteOrder, unsigned char * pabyData, int offset, bool inMercator);

