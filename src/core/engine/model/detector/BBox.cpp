#include "BBox.h"

float BBox::operator&(const BBox &other) const {
    const float intersect_w = std::max(0.0F, std::min(box.br().x, other.box.br().x) - std::max(box.tl().x, other.box.tl().x));
    const float intersect_h = std::max(0.0F, std::min(box.br().y, other.box.br().y) - std::max(box.tl().y, other.box.tl().y));
    const float intersection = intersect_w * intersect_h;
    if (intersection <= 0.0F) return 0.0F;

    const float area_a = box.area();
    const float area_b = other.box.area();
    const float union_area = area_a + area_b - intersection + 1e-6F;
    return intersection / union_area;
}

cv::Point2f BBox::center() const {
    return cv::Point2f(box.x + box.width * 0.5F, box.y + box.height * 0.5F);
}
