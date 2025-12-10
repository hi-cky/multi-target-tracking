#include "Matcher.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace {
// 计算 IoU，直接复用 BBox 的 & 运算符
float IoU(const BBox &a, const BBox &b) { return a & b; }

// 余弦相似度，已在 Feature 内实现
float Cosine(const Feature &a, const Feature &b) { return a.cosine_similarity(b); }
}

Matcher::Matcher(const Config &cfg) : cfg_(cfg) {
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
                const float cos = Cosine(left[i].feature, right[j].feature);
                const float w = (cfg_.iou_weight * iou + cfg_.feature_weight * cos) / norm_;
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

// 工厂方法，默认配置
std::unique_ptr<IMatcher> CreateMatcher(float iou_weight, float feature_weight, float threshold) {
    Matcher::Config cfg{.iou_weight = iou_weight, .feature_weight = feature_weight, .threshold = threshold};
    return std::make_unique<Matcher>(cfg);
}
