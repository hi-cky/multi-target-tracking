#include "OrtEnvSingleton.h"
#include <iostream>
#include <cstdlib>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#endif

namespace {
// ONNX Runtime 在 Windows 下需要 wchar 路径，所以 Windows 下转换成 wstring
#if defined(_WIN32)
std::wstring ToOrtPath(const std::string &path) {
    return std::wstring(path.begin(), path.end());
}
#else
// Linux / macOS 下直接用 string 作为路径
std::string ToOrtPath(const std::string &path) {
    return path;
}
#endif
}  // namespace


Ort::Env& GetOrtEnv() {
    static Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "mt-tracking"};
    return env;
}

std::unique_ptr<Ort::Session> CreateSession(const OrtEnvConfig& config){

    // 3. 创建 ONNX Runtime Session
    auto ort_path = ToOrtPath(config.model_path);
    try {
        Ort::SessionOptions session_opts;
        // 设置优化级别
        session_opts.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        
        // 设置执行线程数（根据CPU核心数）
        unsigned int num_threads = std::thread::hardware_concurrency();
        if (num_threads > 0) {
            session_opts.SetIntraOpNumThreads(num_threads);
            session_opts.SetInterOpNumThreads(num_threads);
        }
        
        // 启用CPU Memory Arena以提高性能
        session_opts.EnableCpuMemArena();
        
        
        if (config.using_gpu) { 
            OrtCUDAProviderOptions cuda_options{};
            cuda_options.device_id = config.device_id;
            cuda_options.arena_extend_strategy = 0;  // kNextPowerOfTwo
            cuda_options.gpu_mem_limit = config.gpu_mem_limit;  // 2GB
            // 使用正确的枚举值（根据ONNX Runtime头文件定义）
            cuda_options.cudnn_conv_algo_search = OrtCudnnConvAlgoSearchExhaustive;
            cuda_options.do_copy_in_default_stream = 1;
            
            session_opts.AppendExecutionProvider_CUDA(cuda_options);
        }
        return std::make_unique<Ort::Session>(GetOrtEnv(), ort_path.c_str(), session_opts);
        
    } catch (const Ort::Exception& e) {
        std::cerr << "[ERROR] 模型加载失败: " << e.what() << std::endl;
        throw std::runtime_error("FeatureExtractor: 模型加载失败。请检查 CUDA 环境和相关配置是否正常！");
    }
}