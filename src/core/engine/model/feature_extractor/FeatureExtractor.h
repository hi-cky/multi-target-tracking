#pragma once

#include "IFeatureExtractor.h"

#include <onnxruntime_cxx_api.h>
#include <opencv2/imgproc.hpp>

#include <string>
#include <vector>

// OSNet-ONNX 特征提取器实现，输出 L2 归一化向量
class FeatureExtractor : public IFeatureExtractor {
public:
    FeatureExtractor(const FeatureExtractorConfig &cfg);
    std::vector<float> extract(const cv::Mat &patch);

private:
    FeatureExtractorConfig config_;
    std::unique_ptr<Ort::Session> session_;
    std::string input_name_;
    std::string output_name_;
    std::vector<int64_t> input_shape_;
};