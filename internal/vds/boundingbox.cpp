#include "boundingbox.h"

namespace openvds_adapter {

std::vector< std::pair<int, int> > BoundingBox::index() noexcept (true) {
    return { {0, 0}, {this->ils, 0}, {this->ils, this->xls}, {0, this->xls} };
}

std::vector< std::pair<double, double> > BoundingBox::world() noexcept (true) {
    std::vector< std::pair<double, double> > world_points;

    auto points = this->index();
    std::for_each(points.begin(), points.end(),
        [&](const std::pair<int, int>& point) {
            auto p = this->transformer.IJKIndexToWorld(
                { point.first, point.second, 0 }
            );
            world_points.emplace_back(p[0], p[1]);
        }
    );

    return world_points;
};

std::vector< std::pair<int, int> > BoundingBox::annotation() noexcept (true) {
    auto points = this->index();
    std::transform(points.begin(), points.end(), points.begin(),
        [this](std::pair<int, int>& point) {
            auto anno = this->transformer.IJKIndexToAnnotation({
                point.first,
                point.second,
                0
            });
            return std::pair<int, int>{anno[0], anno[1]};
        }
    );

    return points;
};

} /* namespace openvds_adapter */
