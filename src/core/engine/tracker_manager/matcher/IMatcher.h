#pragma once

#include <utility>
#include <vector>

#include "../Tracker.h"

struct MatcherConfig {
    float iou_weight = 0.5f;
    float feature_weight = 0.5f;
    float threshold = 0.5f;  // 加权得分达到阈值视为匹配
};

class IMatcher {
public:
    virtual ~IMatcher() = default;
    
    // 对输入的两组 TrackerInner 进行匹配，输出匹配成功的序号对列表
    virtual std::vector<std::pair<int, int>> match(
        const std::vector<TrackerInner>& left,
        const std::vector<TrackerInner>& right
    ) = 0;
};
