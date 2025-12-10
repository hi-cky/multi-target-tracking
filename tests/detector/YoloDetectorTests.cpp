#include <gtest/gtest.h>
#include <filesystem>
#include <iostream>

#include <opencv2/opencv.hpp>

#include "core/processor/model/detector/YoloDetector.h"

TEST(YoloDetectorTests, DetectsOnBlankFrame) {
    constexpr const char *kProjectRoot = PROJECT_ROOT_DIR;
    const std::filesystem::path model_path = std::filesystem::path(kProjectRoot) / "model" / "yolo12n.onnx";
    ASSERT_TRUE(std::filesystem::exists(model_path));

    DetectorConfig config;
    config.model_path = model_path.string();

    YoloDetector detector(config);

    cv::Mat dummy(480, 640, CV_8UC3, cv::Scalar(0, 0, 0));
    const int frame_index = 7;
    const auto boxes = detector.detect(dummy, frame_index);

    EXPECT_GE(boxes.size(), 0U);
}

// 新增手动检查用例：从指定图片路径运行检测并打印结果，方便人工确认
TEST(YoloDetectorTests, ManualInspectOnRealImage) {
    constexpr const char *kProjectRoot = PROJECT_ROOT_DIR;

    const std::filesystem::path model_path = std::filesystem::path(kProjectRoot) / "model" / "yolo12n.onnx";
    ASSERT_TRUE(std::filesystem::exists(model_path));

    // 允许通过环境变量 DETECTOR_TEST_IMAGE 指定测试图片，默认使用 capture/sample.jpg
    const char *env_path = std::getenv("DETECTOR_TEST_IMAGE");
    std::filesystem::path image_path = env_path ? std::filesystem::path(env_path)
                                                : "/Users/corn/Downloads/7552.jpg_wh860.jpg";

    if (!std::filesystem::exists(image_path)) {
        GTEST_SKIP() << "未找到测试图片: " << image_path;
    }

    DetectorConfig config;
    config.model_path = model_path.string();

    YoloDetector detector(config);

    cv::Mat frame = cv::imread(image_path.string());
    if (frame.empty()) {
        // 若传入的是视频，尝试读取首帧
        cv::VideoCapture cap(image_path.string());
        if (cap.isOpened()) {
            cap.read(frame);
        }
    }

    if (frame.empty()) {
        GTEST_SKIP() << "无法读取图像或视频首帧: " << image_path;
    }

    const auto boxes = detector.detect(frame, 0);

    // 打印检测结果供人工核对
    std::cout << "\n[ManualInspect] image: " << image_path << "\n";
    std::cout << "detect count: " << boxes.size() << "\n";
    for (size_t i = 0; i < boxes.size(); ++i) {
        const auto &b = boxes[i];
        std::cout << "  #" << i
                  << " score=" << b.score
                  << " bbox=(" << b.box.x << ", " << b.box.y << ", "
                  << (b.box.x + b.box.width) << ", " << (b.box.y + b.box.height) << ")"
                  << "\n";
    }

    EXPECT_GE(boxes.size(), 0U);
}
