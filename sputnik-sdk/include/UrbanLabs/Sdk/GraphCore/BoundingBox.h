#pragma once

#include <UrbanLabs/Sdk/GraphCore/Point.h>
#include <UrbanLabs/Sdk/Output/JsonXMLFormatter.h>

class BoundingBox {
private:
    Point hiLeft_, loRight_;
public:
    BoundingBox();
    BoundingBox(const Point &p);
    BoundingBox(const Point &hl, const Point &lr);

    void update(const Point &p);
    Point::CoordType getMinLat();
    Point::CoordType getMaxLat();
    Point::CoordType getMinLon();
    Point::CoordType getMaxLon();

    Point getHiLeft() const;
    Point getLoRight() const;
    Point getCenter() const;

    JSONFormatterNode toJSONFormatterNode();
};
