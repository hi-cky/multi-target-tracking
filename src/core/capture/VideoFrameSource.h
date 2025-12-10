#pragma once

#include "core/capture/IImageIterator.h"
#include <opencv2/videoio.hpp>
#include <memory>
#include <string>

class VideoFileIterator : public IImageIterator {
public:
    explicit VideoFileIterator(const std::string &path);
    bool hasNext() const override;
    bool next(cv::Mat &frame) override;
private:
    cv::VideoCapture cap_;
    bool finished_ = false;
};

class VideoFrameSource {
public:
    explicit VideoFrameSource(const std::string &path);
    std::unique_ptr<IImageIterator> createIterator() const;
private:
    std::string path_;
};
