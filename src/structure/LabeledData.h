#pragma once

#include <opencv2/core.hpp>
#include <vector>

// 统一的多目标追踪标注结构体，供检测/追踪/可视化模块共用
struct LabeledObject {
    int id = -1;                   // 每个目标的稳定 ID
    cv::Rect bbox;                 // 像素级别的检测框
    int class_id = -1;             // 模型预测类别（仅做人时固定为 0）
    float score = 0.0F;            // 置信度打分
};

// 封装单帧全部检测结果，便于下游消费
struct LabeledFrame {
    int frame_index = 0;                   // 该帧在视频中的索引
    std::vector<LabeledObject> objs;       // 该帧内的全部目标
};
