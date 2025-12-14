#pragma once

#include <memory>
#include <vector>

#include "Tracker.h"
#include "structure/LabeledData.h"
#include "../matcher/IMatcher.h"

struct TrackerManagerConfig {
    float iou_weight = 0.5f;
    float feature_weight = 0.5f;
    float match_threshold = 0.5f;
    int max_life = 30;
    float feature_momentum = 0.7f;
};

class TrackerManager {
public:
    explicit TrackerManager(const TrackerManagerConfig &cfg = {});

    // 预测所有轨迹
    void predictAll();
    // 输入检测结果，更新匹配；返回存活的轨迹列表
    const std::vector<std::unique_ptr<Tracker>> &update(const std::vector<TrackerInner> &detections);

    // 中文注释：获取当前所有 Tracker 的“预测/更新后”结果，并组装成统一的 LabeledFrame
    // 说明：该方法只负责把 trackers_ 的当前状态导出；Tracker 的 predict/update 仍由外部时序控制。
    void fillLabeledFrame(int frame_index, LabeledFrame &outFrame) const;

    const std::vector<std::unique_ptr<Tracker>> &trackers() const { return trackers_; }

private:
    TrackerManagerConfig cfg_;
    std::unique_ptr<IMatcher> matcher_;
    std::vector<std::unique_ptr<Tracker>> trackers_;
    size_t next_id_ = 0;
};
