#pragma once

#include <vector>

#include "IMatcher.h"

// IoU+特征余弦加权匹配器
class Matcher : public IMatcher {
public:
    explicit Matcher(const MatcherConfig &cfg);
    std::vector<std::pair<int, int>> match(const std::vector<TrackerInner> &left,
                                          const std::vector<TrackerInner> &right) override;

private:
    MatcherConfig cfg_;
    float norm_ = 1.0f;
};
