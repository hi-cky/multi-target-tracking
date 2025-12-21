#pragma once

#include "BBox.h"
#include <opencv2/core.hpp>
#include <vector>
#include "../OrtEnvSingleton.h"

// 统一的检测器配置，便于在 UI 或配置文件里集中调整
struct DetectorConfig {
    int input_width = 640;       // 模型期望的输入宽度
    int input_height = 640;      // 模型期望的输入高度
    float score_threshold = 0.5F;   // objectness 与类别融合后的阈值
    float nms_threshold = 0.8F;     // 同类别框的 IoM NMS 阈值
    bool filter_edge_boxes = true; // 是否过滤触边框（某边位于或超过画面边界）
    // 检测器关注的类别 ID 列表；为空表示不过滤（接受所有类别）
    // 说明：我们在推理热路径里会把它预处理成 unordered_set 来做 O(1) 判断，避免逐个遍历。
    std::vector<int> focus_class_ids = {};

    OrtEnvConfig ort_env_config;
};

// 检测器基类，确保不同模型都能输出统一的数据结构
class IDetector {
public:
    virtual ~IDetector() = default;

    // 对输入帧做检测并输出结构化结果
    virtual std::vector<BBox> detect(const cv::Mat &frame, int frame_index) = 0;
};
