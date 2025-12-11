#pragma once

#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

#include "IDetector.h"

// YOLOv12n ONNX 推理封装，负责加载模型与输出检测结果
class YoloDetector : public IDetector {
public:
    explicit YoloDetector(const DetectorConfig &config);
    ~YoloDetector() override = default;

    std::vector<BBox> detect(const cv::Mat &frame, int frame_index) override;

private:
    struct PreprocessResult {
        std::vector<float> tensor;   // 按 NCHW 排列的输入张量
        float scale = 1.0F;          // letterbox 缩放比例
        float pad_x = 0.0F;          // x 方向填充像素
        float pad_y = 0.0F;          // y 方向填充像素
    };

    PreprocessResult preprocess(const cv::Mat &frame) const;
    std::vector<BBox> runInference(const PreprocessResult &prep, const cv::Size &original_size) const;
    static std::vector<BBox> applyNms(const std::vector<BBox> &boxes, float iou_threshold);

    DetectorConfig config_;
    std::unique_ptr<Ort::Session> session_;  // 推理会话实例
    std::string input_name_;
    std::vector<std::string> output_names_;
    std::vector<int64_t> input_shape_;
};
