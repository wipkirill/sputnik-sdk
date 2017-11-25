#pragma once

#include <cstdint>
#include <UrbanLabs/Sdk/Platform/Stdafx.h>

/**
 * http://www.vanwal.dk/freek/pubs/esa08_presentation.pdf
 * @brief The BalancedPeanoCurve class
 */
class BalancedPeanoCurve {
private:
    int levels_;
    double width_;
private:
    const static double M_SQRT_3;
    double normalize(double x);
    BalancedPeanoCurve(double width, int levels);
public:
    BalancedPeanoCurve();
    int64_t convert(double x, double y);
};
