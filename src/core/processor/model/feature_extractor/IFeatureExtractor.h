#pragma once

#include <opencv2/core.hpp>
#include "../OrtEnvSingleton.h"
#include <vector>

// 特征提取配置，便于统一调整输入尺寸和模型路径
struct FeatureExtractorConfig {
    int input_height = 256;     // 模型期望的输入高度
    int input_width = 128;      // 模型期望的输入宽度
    
    OrtEnvConfig ort_env_config;
};


// 特征提取器基类，输出一维向量用于 ReID / 匹配
class IFeatureExtractor {
public:
    virtual ~IFeatureExtractor() = default;

    // 从输入图像提取归一化后的特征向量
    virtual std::vector<float> extract(const cv::Mat &patch) = 0;
};
