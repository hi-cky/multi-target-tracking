#pragma once

#include <algorithm>
#include <cmath>

#include <opencv2/core.hpp>

// ROI 配置（归一化坐标系，取值范围 0~1）
// - (x,y) 表示 ROI 左上角占原图的百分比位置
// - (w,h) 表示 ROI 宽高占原图的百分比
// - enabled=false 表示使用整帧
struct RoiConfig {
    bool enabled = false;
    float x = 0.0F;
    float y = 0.0F;
    float w = 1.0F;
    float h = 1.0F;
};

// 将归一化 ROI 转为像素 Rect，并做边界裁剪；若 ROI 非法则返回空 Rect
inline cv::Rect RoiToPixelRect(const RoiConfig &roi, const cv::Size &frame_size) {
    if (!roi.enabled) return cv::Rect();
    if (frame_size.width <= 0 || frame_size.height <= 0) return cv::Rect();

    const float x = std::clamp(roi.x, 0.0F, 1.0F);
    const float y = std::clamp(roi.y, 0.0F, 1.0F);
    const float w = std::clamp(roi.w, 0.0F, 1.0F);
    const float h = std::clamp(roi.h, 0.0F, 1.0F);
    if (w <= 0.0F || h <= 0.0F) return cv::Rect();

    const int px = static_cast<int>(std::round(x * static_cast<float>(frame_size.width)));
    const int py = static_cast<int>(std::round(y * static_cast<float>(frame_size.height)));
    const int pw = static_cast<int>(std::round(w * static_cast<float>(frame_size.width)));
    const int ph = static_cast<int>(std::round(h * static_cast<float>(frame_size.height)));

    cv::Rect r(px, py, pw, ph);
    r &= cv::Rect(0, 0, frame_size.width, frame_size.height);
    if (r.width <= 0 || r.height <= 0) return cv::Rect();
    return r;
}

