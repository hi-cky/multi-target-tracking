#include "FeatureExtractor.h"
#include "Feature.h"
#include <onnxruntime_cxx_api.h>
#include <opencv2/imgproc.hpp>
#include <filesystem>

// OSNet-ONNX 特征提取器实现，输出 L2 归一化向量

FeatureExtractor::FeatureExtractor(const FeatureExtractorConfig &cfg)
    : config_(cfg) {
  // 1. 检查模型路径是否正确
  std::string model_path = config_.ort_env_config.model_path;
  if (model_path.empty()) {
    throw std::invalid_argument("FeatureExtractor: 模型路径为空");
  }
  if (!std::filesystem::exists(model_path)) {
    throw std::runtime_error("FeatureExtractor: 模型文件不存在 -> " +
                             model_path);
  }

  // 2. 初始化ort环境
  session_ = CreateSession(cfg.ort_env_config);

  Ort::AllocatorWithDefaultOptions allocator;
  input_name_ = session_->GetInputNameAllocated(0, allocator).get();
  output_name_ = session_->GetOutputNameAllocated(0, allocator).get();

  input_shape_ =
      session_->GetInputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape();
  if (input_shape_.size() != 4) {
    throw std::runtime_error("FeatureExtractor: 输入形状不是 NCHW");
  }
  // 用模型配置覆盖 H/W，保持 batch=1、channel=3
  input_shape_[0] = 1;
  input_shape_[2] = config_.input_height;
  input_shape_[3] = config_.input_width;
}


std::vector<float> FeatureExtractor::extract(const cv::Mat &patch) {
    if (patch.empty()) {
        throw std::invalid_argument("FeatureExtractor: 输入图像为空");
    }
    
    cv::Mat resized;
    cv::resize(patch, resized,
                cv::Size(config_.input_width, config_.input_height));
    cv::cvtColor(resized, resized, cv::COLOR_BGR2RGB);
    resized.convertTo(resized, CV_32F, 1.0 / 255.0);
    
    // ImageNet 均值方差归一化
    const cv::Scalar mean(0.485f, 0.456f, 0.406f);
    const cv::Scalar std(0.229f, 0.224f, 0.225f);
    cv::Mat normalized;
    cv::subtract(resized, mean, normalized);
    cv::divide(normalized, std, normalized);
    
    std::vector<cv::Mat> chw(3);
    cv::split(normalized, chw);
    std::vector<float> input_tensor(3 * config_.input_height * config_.input_width);
    const size_t channel_size =
        static_cast<size_t>(config_.input_height * config_.input_width);
    for (int c = 0; c < 3; ++c) {
        std::memcpy(input_tensor.data() + c * channel_size, chw[c].ptr<float>(),
                    channel_size * sizeof(float));
    }
    
    Ort::MemoryInfo memory_info =
        Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
    Ort::Value input = Ort::Value::CreateTensor<float>(
        memory_info, input_tensor.data(), input_tensor.size(),
        input_shape_.data(), input_shape_.size());
    
    const char *input_names[] = {input_name_.c_str()};
    const char *output_names[] = {output_name_.c_str()};
    auto outputs = session_->Run(Ort::RunOptions{nullptr}, input_names, &input, 1,
                                output_names, 1);
    if (outputs.empty()) {
        throw std::runtime_error("FeatureExtractor: 推理输出为空");
    }
    
    const auto &out_tensor = outputs.front();
    const float *out_data = out_tensor.GetTensorData<float>();
    const auto out_shape = out_tensor.GetTensorTypeAndShapeInfo().GetShape();
    if (out_shape.size() < 2) {
        throw std::runtime_error("FeatureExtractor: 输出形状不正确");
    }
    const size_t feat_dim = static_cast<size_t>(out_shape.back());
    Feature feat(std::vector<float>(out_data, out_data + feat_dim));
    return feat.normalized().values();
}
