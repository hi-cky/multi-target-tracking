#pragma once
#include <onnxruntime_cxx_api.h>

// 特征提取配置，便于统一调整输入尺寸和模型路径
struct OrtEnvConfig {
    std::string model_path;     // ONNX 权重路径
    bool using_gpu = false;             // 是否使用 GPU 加速
    // CUDA配置需要在使用GPU且创建会话时设置
    // 此处仅保留配置标志，仅当设置使用gpu时才会生效
    int device_id = 0;
    size_t gpu_mem_limit = 2ULL * 1024 * 1024 * 1024;  // 2GB
};


std::unique_ptr<Ort::Session> CreateSession(const OrtEnvConfig& config);