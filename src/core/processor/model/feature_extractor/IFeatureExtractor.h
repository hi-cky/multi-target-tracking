#pragma once

#include <memory>
#include <opencv2/core.hpp>
#include <string>
#include <vector>

// 特征提取配置，便于统一调整输入尺寸和模型路径
struct FeatureExtractorConfig {
    std::string model_path;     // ONNX 权重路径
    int input_height = 256;     // 模型期望的输入高度
    int input_width = 128;      // 模型期望的输入宽度
};


// 特征提取器基类，输出一维向量用于 ReID / 匹配
class IFeatureExtractor {
public:
    virtual ~IFeatureExtractor() = default;

    // 从输入图像提取归一化后的特征向量
    virtual std::vector<float> extract(const cv::Mat &patch) = 0;
};

// 便捷创建具体实现的工厂方法
std::unique_ptr<IFeatureExtractor> CreateFeatureExtractor(const FeatureExtractorConfig &config);
