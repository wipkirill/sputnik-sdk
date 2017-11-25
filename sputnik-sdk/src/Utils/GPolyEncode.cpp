#include <UrbanLabs/Sdk/Utils/GPolyEncode.h>

#include <stack>
#include <limits>
#include <sstream>

// Format of serialized geometry
// A single polyline:
//  | 1st geometry |
// A variable number of polylines:
//  | 1st geometry (var length) | separator | 2nd geometry (var length) | ... | nth geometry | separator

using namespace std;

/**
 * @brief encodeNumber
 * @param num
 * @return
 */
inline string encodeNumber(int num)
{
    ostringstream encodeString;

    while (num >= 0x20) {
        int nextValue = (0x20 | (num & 0x1f)) + 63;
        if (!(0 <= nextValue && nextValue <= numeric_limits<char>::max())) {
            throw std::runtime_error("A fatal error when encoding a number");
        }
        encodeString << (static_cast<char>(nextValue));
        num >>= 5;
    }

    num += 63;
    encodeString << (static_cast<char>(num));

    return encodeString.str();
}
/**
 * @brief encodeSignedNumber
 * @param num
 * @return
 */
inline string encodeSignedNumber(int num)
{ // TODO: should be unsigned?
    int signedNum = num << 1;
    if (num < 0) {
        signedNum = ~(signedNum);
    }
    return (encodeNumber(signedNum));
}
/**
 * @decodePoints
 */
string GPolyEncoder::encodePoints(const vector<vector<Point> >& points)
{
    string res;
    for (size_t i = 0; i < points.size(); ++i) {
        res += encodePoints(points[i]);
        res.append(1, SEPARATOR);
    }
    return res;
}
/**
 * @brief GPolyEncoder::encodePoints
 * @param points
 * @return
 */
string GPolyEncoder::encodePoints(const vector<Point>& points)
{
    ostringstream encodedPoints;
    int prevLat = 0, prevLon = 0;
    for (Point pt : points) {
        int lat = pt.lat() * SCALE_FACTOR;
        int lng = pt.lon() * SCALE_FACTOR;

        encodedPoints << encodeSignedNumber(lat - prevLat);
        encodedPoints << encodeSignedNumber(lng - prevLon);

        prevLat = lat;
        prevLon = lng;
    }
    return encodedPoints.str();
}
/**
 * @brief GPolyEncoder::decodePoints
 */
vector<Point> GPolyEncoder::decodePoints(const char* encoded, size_t len)
{
    vector<Point> track;
    int value[2] = { 0, 0 };
    for (int index = 0; index < len;) {
        for (int i = 0; i < 2; ++i) {
            int b = 0, shift = 0, result = 0;
            do {
                if (index >= len) {
                    string msg = "Probably the encoded polyline is broken and cannot be decoded";
                    throw std::runtime_error(msg);
                }
                b = encoded[index++] - 63;
                result |= (b & 0x1f) << shift;
                shift += 5;
            } while (b >= 0x20);
            int diff = ((result & 1) != 0 ? ~(result >> 1) : (result >> 1));
            value[i] += diff;
        }
        track.emplace_back(static_cast<Point::CoordType>(value[0] / SCALE_FACTOR),
                           static_cast<Point::CoordType>(value[1] / SCALE_FACTOR));
    }
    return track;
}
/**
 * @brief decodePoints
 * @param encoded
 * @return
 */
vector<vector<Point> > GPolyEncoder::decodePoints2(const char* encoded, size_t len)
{
    vector<Point> line;
    vector<vector<Point> > track;
    int value[2] = { 0, 0 };
    for (int index = 0; index < len;) {
        for (int i = 0; i < 2; ++i) {
            int b = 0, shift = 0, result = 0;
            do {
                if (index >= len) {
                    const static string msg = "Probably the encoded polyline is broken and cannot be decoded";
                    throw std::runtime_error(msg);
                }
                b = encoded[index++] - 63;
                result |= (b & 0x1f) << shift;
                shift += 5;
            } while (b >= 0x20);

            int diff = ((result & 1) != 0 ? ~(result >> 1) : (result >> 1));
            value[i] += diff;
        }

        line.emplace_back(static_cast<Point::CoordType>(value[0] / SCALE_FACTOR),
                          static_cast<Point::CoordType>(value[1] / SCALE_FACTOR));

        if (index >= len) {
            const static string msg = "Probably the encoded polyline is broken and cannot be decoded";
            throw std::runtime_error(msg);
        }

        if (encoded[index] == SEPARATOR) {
            track.emplace_back(std::move(line));
            value[0] = value[1] = 0;
            ++index;
        }
    }
    return track;
}
