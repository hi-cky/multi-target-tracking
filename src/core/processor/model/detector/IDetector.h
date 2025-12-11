#pragma once

#include "BBox.h"
#include <opencv2/core.hpp>
#include "../OrtEnvSingleton.h"

// 统一的检测器配置，便于在 UI 或配置文件里集中调整
struct DetectorConfig {
    int input_width = 640;       // 模型期望的输入宽度
    int input_height = 640;      // 模型期望的输入高度
    float score_threshold = 0.25F;   // objectness 与类别融合后的阈值
    float nms_threshold = 0.7F;     // 同类别框的 IoU NMS 阈值
    
    OrtEnvConfig ort_env_config;
};

// 检测器基类，确保不同模型都能输出统一的数据结构
class IDetector {
public:
    virtual ~IDetector() = default;

    // 对输入帧做检测并输出结构化结果
    virtual std::vector<BBox> detect(const cv::Mat &frame, int frame_index) = 0;
};
