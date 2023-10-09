#include <limits>
#include <random>

#include "regularsurface.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
namespace {

static Grid samples_10_grid = Grid(2, 0, 7.2111, 3.6056, 33.69);
static constexpr float fill = -999.25;
static constexpr std::size_t nrows = 3;
static constexpr std::size_t ncols = 2;
static std::array<float, nrows *ncols> ref_surface_data = {2, 3, 5, 7, 11, 13};

TEST(AffineTransformationTest, InverseProperty) {
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

TEST(RegularSurfaceSubscriptTest, SingleIndexOutOfRange) {
    RegularSurface surface =
        RegularSurface(ref_surface_data.data(), nrows, ncols, samples_10_grid, fill);

    std::size_t index_lower = 0;
    std::size_t index_upper = ref_surface_data.size() - 1;
    std::size_t index_above_upper = index_upper + 1;

    EXPECT_EQ(surface[index_lower], ref_surface_data[0]) << "Unexpected value";

    EXPECT_EQ(surface[index_upper], ref_surface_data[ref_surface_data.size() - 1])
        << "Unexpected value";

    EXPECT_THAT([&]() { surface[index_above_upper]; },
                testing::ThrowsMessage<std::runtime_error>(
                    testing::HasSubstr("operator[]: index out of range")));
}

TEST(RegularSurfaceSubscriptTest, SingleSubscriptVersusRefValue) {
    RegularSurface surface =
        RegularSurface(ref_surface_data.data(), nrows, ncols, samples_10_grid, fill);

    for (std::size_t i = 0; i < ref_surface_data.size(); i++) {
        EXPECT_EQ(surface[i], ref_surface_data[i]) << "Unexpected value";
    }
}

TEST(RegularSurfaceSubscriptTest, SingleUpdateValue) {
    std::array<float, nrows *ncols> surface_data = ref_surface_data;

    RegularSurface surface =
        RegularSurface(surface_data.data(), nrows, ncols, samples_10_grid, fill);

    for (std::size_t i = 0; i < surface_data.size(); i++) {
        surface[i] *= 2;
    }

    for (std::size_t i = 0; i < surface_data.size(); i++) {
        EXPECT_EQ(surface[i], ref_surface_data[i] * 2)
            << "Unexpected updated value";
    }
}

TEST(RegularSurfaceSubscriptTest, DoubleRowOutOfRange) {
    RegularSurface surface =
        RegularSurface(ref_surface_data.data(), nrows, ncols, samples_10_grid, fill);

    auto lower_row = as_pair(0, 1);
    auto upper_row = as_pair(nrows - 1, 1);
    auto above_upper_row = as_pair(upper_row.first + 1, upper_row.second);

    EXPECT_EQ(surface[lower_row],
              ref_surface_data[lower_row.first * ncols + lower_row.second])
        << "Wrong row number";

    EXPECT_EQ(surface[upper_row],
              ref_surface_data[upper_row.first * ncols + upper_row.second])
        << "Wrong row number";

    EXPECT_THAT([&]() { surface[above_upper_row]; },
                testing::ThrowsMessage<std::runtime_error>(
                    testing::HasSubstr("operator[]: index out of range")));
}

TEST(RegularSurfaceSubscriptTest, DoubleColOutOfRange) {
    RegularSurface surface =
        RegularSurface(ref_surface_data.data(), nrows, ncols, samples_10_grid, fill);

    auto lower_col = as_pair(1, 0);
    auto upper_col = as_pair(1, ncols - 1);
    auto above_upper_col = as_pair(upper_col.first, upper_col.second + 1);

    EXPECT_EQ(surface[lower_col],
              ref_surface_data[lower_col.first * ncols + lower_col.second])
        << "Wrong col number";

    EXPECT_EQ(surface[upper_col],
              ref_surface_data[upper_col.first * ncols + upper_col.second])
        << "Wrong col number";

    EXPECT_THAT([&]() { surface[above_upper_col]; },
                testing::ThrowsMessage<std::runtime_error>(
                    testing::HasSubstr("operator[]: index out of range")));
}

TEST(RegularSurfaceSubscriptTest, DoubleUpdateValues) {
    std::array<float, nrows *ncols> surface_data = ref_surface_data;
    RegularSurface surface =
        RegularSurface(surface_data.data(), nrows, ncols, samples_10_grid, fill);

    for (std::size_t row = 0; row < nrows; row++) {
        for (std::size_t col = 0; col < ncols; col++) {
            surface[as_pair(row, col)] *= 2;
        }
    }

    for (std::size_t row = 0; row < nrows; row++) {
        for (std::size_t col = 0; col < ncols; col++) {
            EXPECT_EQ(surface[as_pair(row, col)],
                      ref_surface_data[row * ncols + col] * 2)
                << "Unexpected value after update";
        }
    }
}

TEST(RegularSurfaceSubscriptTest, DoubleVersusReference) {
    RegularSurface surface =
        RegularSurface(ref_surface_data.data(), nrows, ncols, samples_10_grid, fill);

    for (std::size_t row = 0; row < nrows; row++) {
        for (std::size_t col = 0; col < ncols; col++) {
            auto p = as_pair(row, col);
            EXPECT_EQ(surface[p], ref_surface_data[row * ncols + col])
                << "Unexpected value";
        }
    }
}

} // namespace
