#pragma once

#include <opencv2/core.hpp>

class BBox {
public:
    BBox() = default;
    BBox(const cv::Rect2f &rect, int cls, float sc) : box(rect), class_id(cls), score(sc) {}

    // 计算 IoU，重载 & 运算符，便于 NMS 调用
    float operator&(const BBox &other) const;
    
    float operator&&(const BBox &other) const;

    // 获取中心点坐标
    cv::Point2f center() const;

    cv::Rect2f box;      // 还原到原图坐标系的框
    int class_id = -1;   // 类别索引
    float score = 0.0F;  // 综合置信度
};
