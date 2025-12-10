#pragma once

#include <memory>
#include <vector>

#include "core/processor/matcher/IMatcher.h"

// IoU+特征余弦加权匹配器
class Matcher : public IMatcher {
public:
    struct Config {
        float iou_weight = 0.5f;
        float feature_weight = 0.5f;
        float threshold = 0.5f;  // 加权得分达到阈值视为匹配
    };

    explicit Matcher(const Config &cfg);
    std::vector<std::pair<int, int>> match(const std::vector<TrackerInner> &left,
                                          const std::vector<TrackerInner> &right) override;

private:
    Config cfg_;
    float norm_ = 1.0f;
};

std::unique_ptr<IMatcher> CreateMatcher(float iou_weight,
                                       float feature_weight,
                                       float threshold);
