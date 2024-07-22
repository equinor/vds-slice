#include "test_utils.hpp"

#include "subvolume.hpp"

void DatahandleCubeIntersectionTest::check_value(
    int value, int expected_iline, int expected_xline, int expected_sample
) {
    int sample = value & 0xFF;            // Bits 0-7
    int xline = (value & 0xFF00) >> 8;    // Bits 8-15
    int iline = (value & 0xFF0000) >> 16; // Bits 15-23

    std::string error_message =
        "at iline " + std::to_string(expected_iline) +
        " xline " + std::to_string(expected_xline) +
        " sample " + std::to_string(expected_sample);
    EXPECT_EQ(iline, expected_iline) << error_message;
    EXPECT_EQ(xline, expected_xline) << error_message;
    EXPECT_EQ(sample, expected_sample) << error_message;
}

void DatahandleCubeIntersectionTest::check_slice(
    struct response response_data,
    int low_annotation[],
    int high_annotation[],
    float factor
) {
    std::size_t nr_of_values = (std::size_t)(response_data.size / sizeof(float));

    int low_index[3] = {
        low_annotation[0] / ILINE_STEP,
        low_annotation[1] / XLINE_STEP,
        low_annotation[2] / SAMPLE_STEP
    };

    int high_index[3] = {
        high_annotation[0] / ILINE_STEP + 1,
        high_annotation[1] / XLINE_STEP + 1,
        high_annotation[2] / SAMPLE_STEP + 1
    };

    EXPECT_EQ(nr_of_values, (high_index[0] - low_index[0]) * (high_index[1] - low_index[1]) * (high_index[2] - low_index[2]));

    int counter = 0;
    auto response_samples = (float*)response_data.data;
    for (int il = low_annotation[0]; il <= high_annotation[0]; il += ILINE_STEP) {
        for (int xl = low_annotation[1]; xl <= high_annotation[1]; xl += XLINE_STEP) {
            for (int s = low_annotation[2]; s <= high_annotation[2]; s += SAMPLE_STEP) {
                int value = std::lround(response_samples[counter] / factor);
                check_value(value, il, xl, s);
                counter += 1;
            }
        }
    }
}

void DatahandleCubeIntersectionTest::check_fence(
    struct response response_data,
    std::vector<float> annotated_coordinates,
    int lowest_sample,
    int highest_sample,
    float factor,
    bool fill_flag
) {
    std::size_t nr_of_values = (std::size_t)(response_data.size / sizeof(float));
    std::size_t nr_of_traces = (std::size_t)(annotated_coordinates.size() / 2);

    int lowest_sample_index = lowest_sample / SAMPLE_STEP;
    int highest_sample_index = highest_sample / SAMPLE_STEP + 1;

    EXPECT_EQ(nr_of_values, nr_of_traces * (highest_sample_index - lowest_sample_index));

    int counter = 0;
    float* response = (float*)response_data.data;

    for (int t = 0; t < nr_of_traces; t++) {
        for (int s = lowest_sample; s <= highest_sample; s += SAMPLE_STEP) {
            float value = response[counter];
            if (!fill_flag) {
                int intValue = int((value / factor) + 0.5f);
                check_value(intValue, annotated_coordinates[2 * t], annotated_coordinates[(2 * t) + 1], s);
            } else {
                EXPECT_EQ(value, fill);
            }

            counter += 1;
        }
    }
}

void DatahandleCubeIntersectionTest::check_attribute(
    SurfaceBoundedSubVolume& subvolume,
    int requested_low_annotation[],
    int requested_high_annotation[],
    int expected_low_annotation[],
    int expected_high_annotation[],
    float factor
) {

    // check assumes that margin is stable for the whole grid (requested samples
    // are at least 2 samples from the border)
    auto margin = 2;

    int requested_low_index[3] = {
        requested_low_annotation[0] / ILINE_STEP,
        requested_low_annotation[1] / XLINE_STEP,
        requested_low_annotation[2] / SAMPLE_STEP - margin
    };

    int requested_high_index[3] = {
        requested_high_annotation[0] / ILINE_STEP + 1,
        requested_high_annotation[1] / XLINE_STEP + 1,
        requested_high_annotation[2] / SAMPLE_STEP + 1 + margin
    };

    int expected_low_index[3] = {
        expected_low_annotation[0] / ILINE_STEP,
        expected_low_annotation[1] / XLINE_STEP,
        expected_low_annotation[2] / SAMPLE_STEP - margin
    };

    int expected_high_index[3] = {
        expected_high_annotation[0] / ILINE_STEP + 1,
        expected_high_annotation[1] / XLINE_STEP + 1,
        expected_high_annotation[2] / SAMPLE_STEP + 1 + margin
    };

    std::size_t nr_of_values = subvolume.nsamples(
        0, (requested_high_index[0] - requested_low_index[0]) * (requested_high_index[1] - requested_low_index[1])
    );

    EXPECT_EQ(
        nr_of_values,
        (expected_high_index[0] - expected_low_index[0]) *
            (expected_high_index[1] - expected_low_index[1]) *
            (expected_high_index[2] - expected_low_index[2])
    );

    int counter = 0;
    for (int il = requested_low_index[0]; il < requested_high_index[0]; ++il) {
        for (int xl = requested_low_index[1]; xl < requested_high_index[1]; ++xl) {
            RawSegment rs = subvolume.vertical_segment(counter);
            counter += 1;

            if (il < expected_low_index[0] || il >= expected_high_index[0] ||
                xl < expected_low_index[1] || xl >= expected_high_index[1]) {
                EXPECT_EQ(rs.size(), 0);
                continue;
            }
            int s = expected_low_index[2];
            for (auto it = rs.begin(); it != rs.end(); ++it) {
                int value = int(*it / factor + 0.5f);
                check_value(value, il * ILINE_STEP, xl * XLINE_STEP, s * SAMPLE_STEP);
                s += 1;
            }
            EXPECT_EQ(s, expected_high_index[2]);
        }
    }
}

void DatahandleCubeIntersectionTest::check_attribute(SurfaceBoundedSubVolume& subvolume, int low[], int high[], float factor) {
    return this->check_attribute(subvolume, low, high, low, high, factor);
}
