#pragma once

#include <vector>

#include <opencv2/core.hpp>

// 单个目标的标注
struct LabeledObject
{
    int id;        // 目标 ID
    cv::Rect bbox; // 边界框 (x, y, width, height)
    int class_id;  // 类别编号（如只做人，可以固定为 0）
    float score;   // 置信度（0~1）
};

// 一帧的标注结果
struct LabeledFrame
{
    int frame_index;                 // 帧编号，从 0 开始
    std::vector<LabeledObject> objs; // 该帧中所有目标
};

