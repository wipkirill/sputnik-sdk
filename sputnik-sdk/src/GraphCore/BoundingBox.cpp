#include <algorithm>
#include <UrbanLabs/Sdk/GraphCore/BoundingBox.h>

using namespace std;

/**
 * @brief BoundingBox::BoundingBox
 */
BoundingBox::BoundingBox() : hiLeft_(), loRight_() {;}
/**
 * @brief BoundingBox::BoundingBox
 * @param p
 */
BoundingBox::BoundingBox(const Point &p) {
    hiLeft_ = p, loRight_ = p;
}
/**
 * @brief BoundingBox
 * @param hl
 * @param lr
 */
BoundingBox::BoundingBox(const Point &hl, const Point &lr) : hiLeft_(hl), loRight_(lr) {
    ;
}
/**
 * @brief BoundingBox::update
 * @param p
 */
void BoundingBox::update(const Point &p) {
    hiLeft_.setLat(min(hiLeft_.lat(), p.lat()));
    hiLeft_.setLon(max(hiLeft_.lon(), p.lon()));
    loRight_.setLat(max(hiLeft_.lat(), p.lat()));
    loRight_.setLon(min(hiLeft_.lon(), p.lon()));
}
/**
 * @brief BoundingBox::getMinLat
 * @return
 */
Point::CoordType BoundingBox::getMinLat() {
    return min(hiLeft_.lat(), loRight_.lat());
}
/**
 * @brief BoundingBox::getMaxLat
 * @return
 */
Point::CoordType BoundingBox::getMaxLat() {
    return max(hiLeft_.lat(), loRight_.lat());
}
/**
 * @brief BoundingBox::getMinLon
 * @return
 */
Point::CoordType BoundingBox::getMinLon() {
    return min(hiLeft_.lon(), loRight_.lon());
}
/**
 * @brief BoundingBox::getMaxLon
 * @return
 */
Point::CoordType BoundingBox::getMaxLon() {
    return max(hiLeft_.lon(), loRight_.lon());
}
/**
 * @brief getHiLeft
 * @return
 */
Point BoundingBox::getHiLeft() const {
   return hiLeft_;
}
/**
 * @brief getLoLeft
 * @return
 */
Point BoundingBox::getLoRight() const {
    return loRight_;
}
/**
 * @brief BoundingBox::getCenter
 * @return
 */
Point BoundingBox::getCenter() const {
    Point::CoordType lat = (hiLeft_.lat()+loRight_.lat())/2.0;
    Point::CoordType lon = (hiLeft_.lon()+loRight_.lon())/2.0;
    return Point(lat, lon);
}
/**
 * @brief toJSONFormatterNode
 * @return
 */
JSONFormatterNode BoundingBox::toJSONFormatterNode() {
    vector<vector<Point::CoordType> > coords = {{hiLeft_.lat(), hiLeft_.lon()},
                                                {loRight_.lat(), loRight_.lon()}};
    JSONFormatterNode bbox("bbox", coords);
    return bbox;
}
