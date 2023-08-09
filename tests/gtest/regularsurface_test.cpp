#include <limits>
#include <random>

#include "regularsurface.hpp"

#include "gtest/gtest.h"
namespace
{

TEST(AffineTransformationTest, InverseProperty)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> distribution(-1000000, 1000000);

    auto xinc = distribution(gen);
    if (xinc == 0) {
        xinc = 1;
    }
    auto yinc = distribution(gen);
    if (yinc == 0) {
        yinc = 1;
    }
    auto rot = distribution(gen);
    auto xori = distribution(gen);
    auto yori = distribution(gen);

    auto x = distribution(gen);
    auto y = distribution(gen);

    std::printf(
        "Test runs on: xinc: %.17g, yinc: %.17g, rot: %.17g, xori: %.17g, yori: %.17g, point: (%.17g, %.17g).\n",
        xinc, yinc, rot, xori, yori, x, y);

    auto f = AffineTransformation::from_rotation(xori, yori, xinc, yinc, rot);
    auto f_inv = AffineTransformation::inverse_from_rotation(xori, yori, xinc, yinc, rot);

    Point point{x, y};
    Point f_finv = f * (f_inv * point);
    Point finv_f = f_inv * (f * point);

    EXPECT_NEAR(point.x, f_finv.x, 0.00001) << "f(f_inv(point)).x != point.x";
    EXPECT_NEAR(point.y, f_finv.y, 0.00001) << "f(f_inv(point)).y != point.y";
    EXPECT_NEAR(point.x, finv_f.x, 0.00001) << "f_inv(f(point)).x != point.x";
    EXPECT_NEAR(point.y, finv_f.y, 0.00001) << "f_inv(f(point)).y != point.y";
}

} // namespace
