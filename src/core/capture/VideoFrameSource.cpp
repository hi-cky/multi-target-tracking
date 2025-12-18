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

CameraIterator::CameraIterator(int cameraIndex) : cap_(cameraIndex) {
    if (!cap_.isOpened()) {
        throw std::runtime_error("无法打开摄像头: index=" + std::to_string(cameraIndex));
    }

    // 中文注释：摄像头读取通常是实时流，不会“自然结束”
    // 如果后续需要设置分辨率/FPS，可在这里通过 cap_.set(...) 进行配置。
}

bool CameraIterator::hasNext() const {
    // 中文注释：实时摄像头默认认为一直有下一帧，直到 read 失败
    return !finished_;
}

bool CameraIterator::next(cv::Mat &frame) {
    if (finished_) return false;
    if (!cap_.read(frame) || frame.empty()) {
        finished_ = true;
        return false;
    }
    return true;
}

VideoFrameSource::VideoFrameSource(const std::string &path) : source_(path) {}

VideoFrameSource::VideoFrameSource(int cameraIndex) : source_(cameraIndex) {}

std::unique_ptr<IImageIterator> VideoFrameSource::createIterator() const {
    if (std::holds_alternative<std::string>(source_)) {
        return std::make_unique<VideoFileIterator>(std::get<std::string>(source_));
    }
    return std::make_unique<CameraIterator>(std::get<int>(source_));
}
