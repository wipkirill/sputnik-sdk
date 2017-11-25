#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <memory>

#include <UrbanLabs/Sdk/GraphCore/Point.h>

class GPolyEncoder {
private:
    const static char SEPARATOR = '\t';
public:
    constexpr static double SCALE_FACTOR = 1e5;
    constexpr static double PRECISION = 1/SCALE_FACTOR;
    static_assert(1.0 <= SCALE_FACTOR && SCALE_FACTOR <= 1e5, "Maximum of 5 precision digits is supported");
public:
    static std::string encodePoints(const std::vector<Point>& points);
    static std::string encodePoints(const std::vector<std::vector<Point>>& points);
    static std::vector<Point> decodePoints(const char* encoded, size_t len);
    static std::vector<std::vector<Point> > decodePoints2(const char* encoded, size_t len);
};
