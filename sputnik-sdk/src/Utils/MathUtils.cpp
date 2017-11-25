#include <cmath>
#include <cassert>
#include <UrbanLabs/Sdk/Utils/MathUtils.h>
#include <UrbanLabs/Sdk/Utils/Logger.h>
#include <UrbanLabs/Sdk/Utils/Types.h>

/**
 * @brief M__SQRT_3
 */
const double BalancedPeanoCurve::M_SQRT_3 = sqrt(3.0);
/**
 * @brief BalancedPeanoCurve::BalancedPeanoCurve
 */
BalancedPeanoCurve::BalancedPeanoCurve() {
    levels_ = 30;
    width_ = 360;
}
/**
 * @brief BalancedPeanoCurve::BalancedPeanoCurve
 * @param width
 * @param height
 */
BalancedPeanoCurve::BalancedPeanoCurve(double width, int levels) : levels_(levels), width_(width) {
    if(levels <= 0 || levels > 30) {
        LOGG(Logger::ERROR) << "too many or too few levels" << Logger::FLUSH;
        exit(EXIT_FAILURE);
    }
}
/**
 * @brief normalize
 * @param x
 * @return
 */
double BalancedPeanoCurve::normalize(double x) {
    while(x < 0)
        x += width_;
    while(x > width_)
        x-= width_;
    return x;
}
/**
 * @brief BalancedPeanoCurve::convert
 * @param x
 * @param y
 * @return
 */
int64_t BalancedPeanoCurve::convert(double x, double y) {
    x = normalize(x);
    y = normalize(y);

    int64_t res = 0;
    double w = width_*M_SQRT_3;
    for(int i = 0; i < levels_; i++) {
        assert(x >= 0 && y >= 0);
        assert(x <= w && y <= w/M_SQRT_3);

        double h = w/M_SQRT_3, part = w/3.0;

        res *= 3;
        if(x < part) {
            std::swap(x, y);
        } else if(x < 2.0*part) {
            double xx = x;
            x = h-y;
            y = xx-part;
            res += 1;
        } else if(x <= 3.0*part) {
            double xx = x;
            x = y;
            y = xx-2.0*part;
            res += 2;
        } else {
            assert(false);
        }

        w /= M_SQRT_3;
    }

    return res;
}
