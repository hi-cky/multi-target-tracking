#pragma once

#include "../structure/TrackerInner.h"
#include <utility>
#include <memory>
#include <vector>

class IMatcher {
public:
    virtual ~IMatcher() = default;
    
    // 对输入的两组 TrackerInner 进行匹配，输出匹配成功的序号对列表
    virtual std::vector<std::pair<int, int>> match(const std::vector<TrackerInner>& left,
                                                  const std::vector<TrackerInner>& right) = 0;
};

std::unique_ptr<IMatcher> CreateMatcher(float iou_weight = 0.5f,
                                       float feature_weight = 0.5f,
                                       float threshold = 0.5f);
