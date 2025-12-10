#pragma once

#include <opencv2/core.hpp>

// 图像迭代器接口：按顺序输出 cv::Mat 帧
class IImageIterator {
public:
    virtual ~IImageIterator() = default;
    virtual bool hasNext() const = 0;
    virtual bool next(cv::Mat &frame) = 0;
};
