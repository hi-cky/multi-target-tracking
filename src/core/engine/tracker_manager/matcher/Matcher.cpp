#include "Matcher.h"

#include <algorithm>
#include <stdexcept>
#include <vector>
#include <cmath>

namespace {
// 计算 IoU，直接复用 BBox 的 & 运算符
float IoU(const BBox &a, const BBox &b) { return a & b; }

// 余弦相似度，已在 Feature 内实现
float Cosine(const Feature &a, const Feature &b) { return a.cosine_similarity(b); }
}

Matcher::Matcher(const MatcherConfig &cfg) : cfg_(cfg) {
    const float sum = cfg_.iou_weight + cfg_.feature_weight;
    if (sum <= 1e-6f) {
        throw std::invalid_argument("Matcher: 权重之和不能为 0");
    }
    norm_ = sum;
}

std::vector<std::pair<int, int>> Matcher::match(const std::vector<TrackerInner> &left,
                                               const std::vector<TrackerInner> &right) {
    std::vector<std::tuple<float, int, int>> scores;  // (score, i, j)
    scores.reserve(left.size() * right.size());

    for (int i = 0; i < static_cast<int>(left.size()); ++i) {
        for (int j = 0; j < static_cast<int>(right.size()); ++j) {
            const float iou = IoU(left[i].box, right[j].box);
            float cos = Cosine(left[i].feature, right[j].feature);
            // 余弦相似度 [-1,1] 映射到 [0,1]，避免负值导致匹配异常
            cos = 0.5f * (cos + 1.0f);

            // 攻防：使用加权 noisy-or，高分项会显著抬高整体，降低丢框风险
            const float wi = cfg_.iou_weight / norm_;
            const float wf = cfg_.feature_weight / norm_;
            const float w = 1.0f
                - std::pow(std::max(1.0f - iou, 0.0f), wi)
                * std::pow(std::max(1.0f - cos, 0.0f), wf);
            if (w >= cfg_.threshold) {
                // 将满足阈值条件的匹配分数和索引加入候选列表
                scores.emplace_back(w, i, j);
            }
        }
    }

    // 分数降序，做一对一择优匹配（贪心）
    std::sort(scores.begin(), scores.end(), [](const auto &a, const auto &b) {
        return std::get<0>(a) > std::get<0>(b);
    });

    std::vector<char> used_left(left.size(), 0), used_right(right.size(), 0);
    std::vector<std::pair<int, int>> matches;
    for (const auto &[score, i, j] : scores) {
        if (used_left[i] || used_right[j]) continue;
        used_left[i] = used_right[j] = 1;
        matches.emplace_back(i, j);
    }
    return matches;
}
