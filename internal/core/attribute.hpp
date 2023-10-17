#ifndef VDS_SLICE_ATTRIBUTE_HPP
#define VDS_SLICE_ATTRIBUTE_HPP

#include "regularsurface.hpp"
#include "subvolume.hpp"
#include <memory>
#include <stdexcept>

/* Base class for attribute calculations
 *
 * The base class main role is to centralize buffer writes such that
 * implementations of new attributes, in the form of derived classes, don't
 * have to deal with pointer arithmetic.
 */
class AttributeMap {
public:
    AttributeMap(void* dst, std::size_t size) : dst(dst), size(size) {};

    virtual float compute(ResampledSegment const & segment) noexcept (false) = 0;

    void write(float value, std::size_t index) {
        std::size_t offset = index * sizeof(float);

        if (offset >= this->size) {
            throw std::out_of_range("Attempting write outside attribute buffer");
        }

        memcpy((char*)this->dst + offset, &value, sizeof(float));
    }

    virtual ~AttributeMap() {};
private:
    void*       dst;
    std::size_t size;
};

struct Value final : public AttributeMap {
    Value(void* dst, std::size_t size) : AttributeMap(dst, size) {}

    float compute(ResampledSegment const & segment) noexcept (false) override;
};


class Min final : public AttributeMap {
public:
    Min(void* dst, std::size_t size) : AttributeMap(dst, size) {}

    float compute(ResampledSegment const & segment) noexcept (false) override;
};

class Max final : public AttributeMap {
public:
    Max(void* dst, std::size_t size) : AttributeMap(dst, size) {}

    float compute(ResampledSegment const & segment) noexcept (false) override;
};

class MaxAbs final : public AttributeMap {
public:
    MaxAbs(void* dst, std::size_t size) : AttributeMap(dst, size) {}

    float compute(ResampledSegment const & segment) noexcept (false) override;
};

class Mean final : public AttributeMap {
public:
    Mean(void* dst, std::size_t size) : AttributeMap(dst, size) {}

    float compute(ResampledSegment const & segment) noexcept (false) override;
};

class MeanAbs final : public AttributeMap {
public:
    MeanAbs(void* dst, std::size_t size) : AttributeMap(dst, size) {}

    float compute(ResampledSegment const & segment) noexcept (false) override;
};

class MeanPos final : public AttributeMap {
public:
    MeanPos(void* dst, std::size_t size) : AttributeMap(dst, size) {}

    float compute(ResampledSegment const & segment) noexcept (false) override;
};

class MeanNeg final : public AttributeMap {
public:
    MeanNeg(void* dst, std::size_t size) : AttributeMap(dst, size) {}

    float compute(ResampledSegment const & segment) noexcept (false) override;
};

class Median final : public AttributeMap {
public:
    Median(void* dst, std::size_t size) : AttributeMap(dst, size) {}

    float compute(ResampledSegment const & segment) noexcept (false) override;
};

class Rms final : public AttributeMap {
public:
    Rms(void* dst, std::size_t size) : AttributeMap(dst, size) {}

    float compute(ResampledSegment const & segment) noexcept (false) override;
};

/* Calculated the population variance as we are interested in variance strictly
 * for the data defined by each window.
 */
class Var final : public AttributeMap {
public:
    Var(void* dst, std::size_t size) : AttributeMap(dst, size) {}

    float compute(ResampledSegment const & segment) noexcept (false) override;
};

/* Calculated the population standard deviation as we are interested in
 * standard deviation strictly for the data defined by each window.
 */
class Sd final : public AttributeMap {
public:
    Sd(void* dst, std::size_t size) : AttributeMap(dst, size) {}

    float compute(ResampledSegment const & segment) noexcept (false) override;
};

class SumPos final : public AttributeMap {
public:
    SumPos(void* dst, std::size_t size) : AttributeMap(dst, size) {}

    float compute(ResampledSegment const & segment) noexcept (false) override;
};

class SumNeg final : public AttributeMap {
public:
    SumNeg(void* dst, std::size_t size) : AttributeMap(dst, size) {}

    float compute(ResampledSegment const & segment) noexcept (false) override;
};

void calc_attributes(
    SurfaceBoundedSubVolume const& src_subvolume,
    ResampledSegmentBlueprint const* dst_segment_blueprint,
    std::vector< std::unique_ptr< AttributeMap > >& attrs,
    std::size_t from,
    std::size_t to
) noexcept (false);

#endif /* VDS_SLICE_ATTRIBUTE_HPP */
