#include "core/capture/VideoFrameSource.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
// 根据原始帧率与采样帧率计算步长（>=1）
int calcFrameStep(double source_fps, double sample_fps) {
    if (sample_fps <= 0.0 || source_fps <= 0.0) return 1;
    const int step = static_cast<int>(std::round(source_fps / sample_fps));
    return std::max(1, step);
}
}  // namespace

VideoFileIterator::VideoFileIterator(const std::string &path, double sample_fps)
    : cap_(path), sample_fps_(sample_fps) {
    if (!cap_.isOpened()) {
        throw std::runtime_error("无法打开视频文件: " + path);
    }

    source_fps_ = cap_.get(cv::CAP_PROP_FPS);
    frame_step_ = calcFrameStep(source_fps_, sample_fps_);
    if (sample_fps_ > 0.0 && source_fps_ > 0.0) {
        sample_fps_ = source_fps_ / static_cast<double>(frame_step_);
    }

    const double total = cap_.get(cv::CAP_PROP_FRAME_COUNT);
    if (total > 0.0) {
        total_frames_ = static_cast<int>(total);
        sample_total_frames_ = (total_frames_ + frame_step_ - 1) / frame_step_;
    }
}

bool VideoFileIterator::hasNext() const {
    if (finished_) return false;
    if (sample_total_frames_ > 0 && sample_index_ >= sample_total_frames_) return false;
    return true;
}

bool VideoFileIterator::next(cv::Mat &frame) {
    if (!hasNext()) return false;

    // 先快速跳过 frame_step_-1 帧，再读取一帧作为输出
    for (int i = 0; i < frame_step_ - 1; ++i) {
        if (!cap_.grab()) {
            finished_ = true;
            return false;
        }
    }

    if (!cap_.read(frame) || frame.empty()) {
        finished_ = true;
        return false;
    }
    ++sample_index_;
    return true;
}

FrameSourceInfo VideoFileIterator::info() const {
    FrameSourceInfo info;
    info.is_live = false;
    info.total_frames = sample_total_frames_;
    info.source_fps = source_fps_;
    info.sample_fps = sample_fps_;
    info.frame_step = frame_step_;
    return info;
}

CameraIterator::CameraIterator(int cameraIndex, double sample_fps)
    : cap_(cameraIndex), sample_fps_(sample_fps) {
    if (!cap_.isOpened()) {
        throw std::runtime_error("无法打开摄像头: index=" + std::to_string(cameraIndex));
    }

    // 摄像头读取通常是实时流，不会“自然结束”
    // 如果后续需要设置分辨率/FPS，可在这里通过 cap_.set(...) 进行配置。

    source_fps_ = cap_.get(cv::CAP_PROP_FPS);
    frame_step_ = calcFrameStep(source_fps_, sample_fps_);
    if (sample_fps_ > 0.0 && source_fps_ > 0.0) {
        sample_fps_ = source_fps_ / static_cast<double>(frame_step_);
    }
}

bool CameraIterator::hasNext() const {
    // 实时摄像头默认认为一直有下一帧，直到 read 失败
    return !finished_;
}

bool CameraIterator::next(cv::Mat &frame) {
    if (finished_) return false;

    // 跳过多余帧（若 frame_step_ > 1）
    for (int i = 0; i < frame_step_ - 1; ++i) {
        if (!cap_.grab()) {
            finished_ = true;
            return false;
        }
    }

    if (!cap_.read(frame) || frame.empty()) {
        finished_ = true;
        return false;
    }
    return true;
}

FrameSourceInfo CameraIterator::info() const {
    FrameSourceInfo info;
    info.is_live = true;
    info.total_frames = -1;
    info.source_fps = source_fps_;
    info.sample_fps = sample_fps_;
    info.frame_step = frame_step_;
    return info;
}

VideoFrameSource::VideoFrameSource(const std::string &path, double sample_fps)
    : source_(path), sample_fps_(sample_fps) {}

VideoFrameSource::VideoFrameSource(int cameraIndex, double sample_fps)
    : source_(cameraIndex), sample_fps_(sample_fps) {}

std::unique_ptr<IImageIterator> VideoFrameSource::createIterator() const {
    if (std::holds_alternative<std::string>(source_)) {
        return std::make_unique<VideoFileIterator>(std::get<std::string>(source_), sample_fps_);
    }
    return std::make_unique<CameraIterator>(std::get<int>(source_), sample_fps_);
}
