#pragma once

#include "core/capture/IImageIterator.h"
#include <opencv2/videoio.hpp>
#include <memory>
#include <string>
#include <variant>

class VideoFileIterator : public IImageIterator {
public:
    explicit VideoFileIterator(const std::string &path, double sample_fps = 0.0);
    bool hasNext() const override;
    bool next(cv::Mat &frame) override;
    FrameSourceInfo info() const override;
private:
    cv::VideoCapture cap_;
    bool finished_ = false;
    double source_fps_ = 0.0;
    double sample_fps_ = 0.0;
    int frame_step_ = 1;
    int total_frames_ = -1;
    int sample_total_frames_ = -1;
    int sample_index_ = 0;
};

// 摄像头迭代器（实时读取，不会自然结束；除非读取失败或摄像头被关闭）
class CameraIterator : public IImageIterator {
public:
    explicit CameraIterator(int cameraIndex, double sample_fps = 0.0);
    bool hasNext() const override;
    bool next(cv::Mat &frame) override;
    FrameSourceInfo info() const override;
private:
    cv::VideoCapture cap_;
    bool finished_ = false;
    double source_fps_ = 0.0;
    double sample_fps_ = 0.0;
    int frame_step_ = 1;
};

class VideoFrameSource {
public:
    explicit VideoFrameSource(const std::string &path, double sample_fps = 0.0);
    // 新增摄像头数据源（例如 0 号摄像头）
    explicit VideoFrameSource(int cameraIndex, double sample_fps = 0.0);
    std::unique_ptr<IImageIterator> createIterator() const;
private:
    // 用 variant 表示“文件”或“摄像头”两种数据源
    std::variant<std::string, int> source_;
    double sample_fps_ = 0.0;
};
