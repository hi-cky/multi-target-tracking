#include "core/capture/VideoFrameSource.h"
#include <stdexcept>

VideoFileIterator::VideoFileIterator(const std::string &path) : cap_(path) {
    if (!cap_.isOpened()) {
        throw std::runtime_error("无法打开视频文件: " + path);
    }
}

bool VideoFileIterator::hasNext() const { return !finished_; }

bool VideoFileIterator::next(cv::Mat &frame) {
    if (finished_) return false;
    if (!cap_.read(frame) || frame.empty()) {
        finished_ = true;
        return false;
    }
    return true;
}

VideoFrameSource::VideoFrameSource(const std::string &path) : path_(path) {}

std::unique_ptr<IImageIterator> VideoFrameSource::createIterator() const {
    return std::make_unique<VideoFileIterator>(path_);
}
