#pragma once
#ifndef IMAGE_SOURCE_H
#define IMAGE_SOURCE_H

#include <string>
#include <stdexcept>
#include <opencv2/opencv.hpp>
#include <iostream>

/**
 * @brief 图像迭代器接口：定义如何按顺序获取 cv::Mat 帧
 */
class IImageIterator {
public:
    virtual ~IImageIterator() {}

    /**
     * @brief 返回是否还有下一帧
     */
    virtual bool hasNext() const = 0;

    /**
     * @brief 读取下一帧图像
     * @param frame 输出参数，用于存放读到的这一帧图像
     * @return true 表示成功读取一帧，false 表示没有更多帧
     */
    virtual bool next(cv::Mat& frame) = 0;
};

// ====================================================================

/**
 * @brief 视频文件迭代器：IImageIterator 的具体实现，用于读取视频文件。
 */
class VideoFileIterator : public IImageIterator {
public:
    /**
     * @brief 构造函数，打开指定路径的视频文件。
     * @param path 视频文件路径
     * @throw std::runtime_error 如果无法打开视频文件
     */
    VideoFileIterator(const std::string& path);

    virtual bool hasNext() const override;
    virtual bool next(cv::Mat& frame) override;

private:
    cv::VideoCapture cap; // OpenCV 视频捕获对象
    bool finished;        // 是否已读完所有帧
};

// ====================================================================

/**
 * @brief 视频源：负责根据视频路径创建一个图像迭代器
 */
class VideoFrameSource {
public:
    /**
     * @brief 构造函数：path 为视频文件路径。
     * 注意：设计文档建议在此处或迭代器内部抛出异常 [cite: 13]。
     * @param path 视频文件路径
     */
    VideoFrameSource(const std::string& path);

    /**
     * @brief 创建一个与该视频绑定的图像迭代器。
     * @return IImageIterator* 指针，调用者负责管理其生命周期。
     */
    IImageIterator* createIterator() const;

private:
    std::string videoPath; // 存储视频文件路径
    // 可以在这里添加其他成员变量，如数据源类型（文件/摄像头）等
};

#endif // IMAGE_SOURCE_H
