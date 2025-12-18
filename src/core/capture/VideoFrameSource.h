#pragma once

#include "core/capture/IImageIterator.h"
#include <opencv2/videoio.hpp>
#include <memory>
#include <string>
#include <variant>

class VideoFileIterator : public IImageIterator {
public:
    explicit VideoFileIterator(const std::string &path);
    bool hasNext() const override;
    bool next(cv::Mat &frame) override;
private:
    cv::VideoCapture cap_;
    bool finished_ = false;
};

// 中文注释：摄像头迭代器（实时读取，不会自然结束；除非读取失败或摄像头被关闭）
class CameraIterator : public IImageIterator {
public:
    explicit CameraIterator(int cameraIndex);
    bool hasNext() const override;
    bool next(cv::Mat &frame) override;
private:
    cv::VideoCapture cap_;
    bool finished_ = false;
};

class VideoFrameSource {
public:
    explicit VideoFrameSource(const std::string &path);
    // 中文注释：新增摄像头数据源（例如 0 号摄像头）
    explicit VideoFrameSource(int cameraIndex);
    std::unique_ptr<IImageIterator> createIterator() const;
private:
    // 中文注释：用 variant 表示“文件”或“摄像头”两种数据源
    std::variant<std::string, int> source_;
};
