#include "subvolume.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
namespace {

TEST(FreeFunctions, FmodWithTolerance) {
    EXPECT_EQ(0, fmod_with_tolerance(4, 2));
    EXPECT_EQ(0, fmod_with_tolerance(4, 0.1));
    EXPECT_EQ(0, fmod_with_tolerance(4, 0.2));
    EXPECT_EQ(0, fmod_with_tolerance(30, 0.3));
}

TEST(FreeFunctions, FloorWithTolerance) {
    EXPECT_EQ(4, floor_with_tolerance(4.0001));
    EXPECT_EQ(4, floor_with_tolerance(4.001));
    EXPECT_EQ(4, floor_with_tolerance(4.002));
    EXPECT_EQ(4, floor_with_tolerance(4.998));
    EXPECT_EQ(5, floor_with_tolerance(4.999));
    EXPECT_EQ(5, floor_with_tolerance(4.9999));
}

TEST(FreeFunctions, CeilWithTolerance) {
    EXPECT_EQ(4, ceil_with_tolerance(4.0001));
    EXPECT_EQ(4, ceil_with_tolerance(4.001));
    EXPECT_EQ(5, ceil_with_tolerance(4.002));
    EXPECT_EQ(5, ceil_with_tolerance(4.998));
    EXPECT_EQ(5, ceil_with_tolerance(4.999));
    EXPECT_EQ(5, ceil_with_tolerance(4.9999));
}

} // namespace
