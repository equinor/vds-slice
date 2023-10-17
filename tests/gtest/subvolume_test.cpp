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

TEST(SegmentBlueprintTest, BoundariesOnSamples) {
    /*
     * 0 2   6   10  14  18  22  26  30  34
     * --*---*---*---*---*---*---*---*---*---
     *           |       |       |
     *          top  reference bottom
     */

    RawSegmentBlueprint raw = RawSegmentBlueprint(4, 2);
    ResampledSegmentBlueprint resampled = ResampledSegmentBlueprint(4);

    float reference = 18;
    float top_boundary = 10;
    float bottom_boundary = 26;

    EXPECT_EQ(2, resampled.nsamples_above(reference, top_boundary));

    EXPECT_EQ(2, raw.top_sample_position(top_boundary));
    EXPECT_EQ(10, resampled.top_sample_position(reference, top_boundary));

    EXPECT_EQ(34, raw.bottom_sample_position(bottom_boundary));
    EXPECT_EQ(26, resampled.bottom_sample_position(reference, bottom_boundary));

    EXPECT_EQ(9, raw.size(top_boundary, bottom_boundary));
    EXPECT_EQ(5, resampled.size(reference, top_boundary, bottom_boundary));
}

TEST(SegmentBlueprintTest, TopBoundaryOnSample) {
    /*
     * 0 2   6   10  14  18  22  26  30  34
     * --*---*---*---*---*---*---*---*---*---
     *           |       |      |
     *          top reference bottom
     */

    RawSegmentBlueprint raw = RawSegmentBlueprint(4, 2);
    ResampledSegmentBlueprint resampled = ResampledSegmentBlueprint(4);

    float reference = 18;
    float top_boundary = 10;
    float bottom_boundary = 25;

    EXPECT_EQ(2, resampled.nsamples_above(reference, top_boundary));

    EXPECT_EQ(2, raw.top_sample_position(top_boundary));
    EXPECT_EQ(10, resampled.top_sample_position(reference, top_boundary));

    EXPECT_EQ(30, raw.bottom_sample_position(bottom_boundary));
    EXPECT_EQ(22, resampled.bottom_sample_position(reference, bottom_boundary));

    EXPECT_EQ(8, raw.size(top_boundary, bottom_boundary));
    EXPECT_EQ(4, resampled.size(reference, top_boundary, bottom_boundary));
}

TEST(SegmentBlueprintTest, BottomBoundaryOnSample) {
    /*
     * 0 2   6   10  14  18  22  26  30  34
     * --*---*---*---*---*---*---*---*---*---
     *            |      |       |
     *           top reference bottom
     */

    RawSegmentBlueprint raw = RawSegmentBlueprint(4, 2);
    ResampledSegmentBlueprint resampled = ResampledSegmentBlueprint(4);

    float reference = 18;
    float top_boundary = 11;
    float bottom_boundary = 26;

    EXPECT_EQ(1, resampled.nsamples_above(reference, top_boundary));

    EXPECT_EQ(6, raw.top_sample_position(top_boundary));
    EXPECT_EQ(14, resampled.top_sample_position(reference, top_boundary));

    EXPECT_EQ(34, raw.bottom_sample_position(bottom_boundary));
    EXPECT_EQ(26, resampled.bottom_sample_position(reference, bottom_boundary));

    EXPECT_EQ(8, raw.size(top_boundary, bottom_boundary));
    EXPECT_EQ(4, resampled.size(reference, top_boundary, bottom_boundary));
}

TEST(SegmentBlueprintTest, NoBoundaryOnSample) {
    /*
     * 0 2   6   10  14  18  22  26  30  34
     * --*---*---*---*---*---*---*---*---*---
     *            |      |      |
     *           top reference bottom
     */

    RawSegmentBlueprint raw = RawSegmentBlueprint(4, 2);
    ResampledSegmentBlueprint resampled = ResampledSegmentBlueprint(4);

    float reference = 18;
    float top_boundary = 11;
    float bottom_boundary = 25;

    EXPECT_EQ(1, resampled.nsamples_above(reference, top_boundary));

    EXPECT_EQ(6, raw.top_sample_position(top_boundary));
    EXPECT_EQ(14, resampled.top_sample_position(reference, top_boundary));

    EXPECT_EQ(30, raw.bottom_sample_position(bottom_boundary));
    EXPECT_EQ(22, resampled.bottom_sample_position(reference, bottom_boundary));

    EXPECT_EQ(7, raw.size(top_boundary, bottom_boundary));
    EXPECT_EQ(3, resampled.size(reference, top_boundary, bottom_boundary));
}

TEST(SegmentBlueprintTest, ReferenceOutsideSample) {
    /*
     * 0 2   6   10  14  18  22  26  30  34
     * --*---*---*---*---*---*---*---*---*---
     *           |      |        |
     *          top  reference bottom
     */

    RawSegmentBlueprint raw = RawSegmentBlueprint(4, 2);

    float reference = 17;
    float top_boundary = 10;
    float bottom_boundary = 26;

    EXPECT_EQ(2, raw.top_sample_position(top_boundary));
    EXPECT_EQ(34, raw.bottom_sample_position(bottom_boundary));
    EXPECT_EQ(9, raw.size(top_boundary, bottom_boundary));
}

TEST(SegmentBlueprintTest, Subsampling01) {
    /*
     * 0 2   6   10  14  18  22  26  30  34
     * --*---*---*---*---*---*---*---*---*---
     *           |       |       |
     *          top  reference bottom
     */

    ResampledSegmentBlueprint resampled = ResampledSegmentBlueprint(0.1);

    float reference = 18;
    float top_boundary = 10;
    float bottom_boundary = 26;

    EXPECT_EQ(80, resampled.nsamples_above(reference, top_boundary));
    EXPECT_EQ(10, resampled.top_sample_position(reference, top_boundary));
    EXPECT_EQ(26, resampled.bottom_sample_position(reference, bottom_boundary));
    EXPECT_EQ(161, resampled.size(reference, top_boundary, bottom_boundary));
}

TEST(SegmentBlueprintTest, Subsampling02) {
    /*
     * 0 2   6   10  14  18  22  26  30  34
     * --*---*---*---*---*---*---*---*---*---
     *           |       |       |
     *          top  reference bottom
     */

    ResampledSegmentBlueprint resampled = ResampledSegmentBlueprint(0.2);

    float reference = 18;
    float top_boundary = 10;
    float bottom_boundary = 26;

    EXPECT_EQ(40, resampled.nsamples_above(reference, top_boundary));
    EXPECT_EQ(10, resampled.top_sample_position(reference, top_boundary));
    EXPECT_EQ(26, resampled.bottom_sample_position(reference, bottom_boundary));
    EXPECT_EQ(81, resampled.size(reference, top_boundary, bottom_boundary));
}

TEST(SegmentBlueprintTest, Subsampling03) {
    /*
     * 0 2   6   10  14  18  22  26  30  34
     * --*---*---*---*---*---*---*---*---*---
     *          |        |           |
     *         top   reference     bottom
     */

    ResampledSegmentBlueprint resampled = ResampledSegmentBlueprint(0.3);

    float reference = 18;
    float top_boundary = 9;
    float bottom_boundary = 30;

    EXPECT_EQ(30, resampled.nsamples_above(reference, top_boundary));
    EXPECT_EQ(9, resampled.top_sample_position(reference, top_boundary));
    EXPECT_FLOAT_EQ(30, resampled.bottom_sample_position(reference, bottom_boundary));
    EXPECT_EQ(71, resampled.size(reference, top_boundary, bottom_boundary));
}

TEST(SegmentBlueprintTest, Subsampling04) {
    /*
     * It is difficult to find a test example where we would end up with value a
     * little bit bigger than the one we want to to make sure we need modified
     * ceiling function. To test this we go into floating point values, which we
     * are not sure ever appear in the seismic vds files, but can in theory
     */

    ResampledSegmentBlueprint resampled = ResampledSegmentBlueprint(0.4);

    float reference = 1;
    float top_boundary = 0.6;
    float bottom_boundary = 1.1;

    EXPECT_EQ(1, resampled.nsamples_above(reference, top_boundary));
    EXPECT_FLOAT_EQ(0.6, resampled.top_sample_position(reference, top_boundary));
    EXPECT_EQ(1, resampled.bottom_sample_position(reference, bottom_boundary));
    EXPECT_EQ(2, resampled.size(reference, top_boundary, bottom_boundary));
}

} // namespace
