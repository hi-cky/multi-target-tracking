#pragma once

#include <opencv2/core.hpp>

// 帧源基础信息（用于 UI 进度、采样参数回显等）
struct FrameSourceInfo {
    bool is_live = false;      // 是否为实时源（摄像头）
    int total_frames = -1;     // 采样后的总帧数（未知时为 -1）
    double source_fps = 0.0;   // 原始帧率（未知时为 0）
    double sample_fps = 0.0;   // 采样帧率（<=0 表示不采样）
    int frame_step = 1;        // 采样步长（>=1）
};

// 图像迭代器接口：按顺序输出 cv::Mat 帧
class IImageIterator {
public:
    virtual ~IImageIterator() = default;
    virtual bool hasNext() const = 0;
    virtual bool next(cv::Mat &frame) = 0;

    // 可选的帧源信息（默认返回空信息）
    virtual FrameSourceInfo info() const { return FrameSourceInfo{}; }
};
