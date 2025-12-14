#include "ImageSource.h"

// ====================================================================
// VideoFileIterator 实现
// ====================================================================

VideoFileIterator::VideoFileIterator(const std::string& path)
    : cap(path), finished(false) // 尝试用路径打开视频
{
    // 检查是否成功打开视频
    if (!cap.isOpened()) {
        // 视频无法打开时，需要抛出 std::runtime_error 异常 [cite: 9, 29]
        throw std::runtime_error("Cannot open video file: " + path);
    }
}

bool VideoFileIterator::hasNext() const {
    // 只要没有标记为结束，就认为还有下一帧 [cite: 15]
    return !finished;
}

bool VideoFileIterator::next(cv::Mat& frame) {
    if (finished) {
        return false;
    }

    // 尝试读取下一帧 [cite: 15]
    bool ok = cap.read(frame);

    // 如果读取失败 (ok==false) 或读到的图像为空 (frame.empty())，则认为已到达末尾 [cite: 15]
    if (!ok || frame.empty()) {
        finished = true; // 标记结束
        return false;
    }

    return true; // 成功读取
}

// ====================================================================
// VideoFrameSource 实现
// ====================================================================

// 构造函数：存储路径 [cite: 15]
VideoFrameSource::VideoFrameSource(const std::string& path)
    : videoPath(path)
{
    // 设计文档中，VideoFrameSource 的构造函数仅存储路径，
    // 实际打开和检查在 createIterator 返回的迭代器中完成 [cite: 15]。
}

// 创建并返回新的 VideoFileIterator 实例 [cite: 15]
IImageIterator* VideoFrameSource::createIterator() const {
    // 由调用者负责管理返回的指针生命周期 [cite: 15]
    return new VideoFileIterator(videoPath);
}