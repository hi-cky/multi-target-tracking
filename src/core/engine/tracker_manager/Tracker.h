#pragma once

#include <opencv2/video/tracking.hpp>
#include "../model/detector/BBox.h"
#include "../model/feature_extractor/Feature.h"

struct TrackerInner {
    BBox box;
    Feature feature;
};

struct TrackerConfig {
    int max_life = 90;          // life 上限
    float feature_momentum = 0.7f; // 新特征的占比（EMA）
};

class Tracker {
public:
    Tracker(size_t id, const TrackerInner &inner, const TrackerConfig &cfg);

    // 卡尔曼预测，更新内部 bbox 但不改变 life
    void predict();
    // 命中更新，返回是否仍存活（恒为 true，便于链式调用）
    bool updateAsHitting(const TrackerInner &detection);
    // 未命中更新，life 减少；若归零返回 true 表示应清除
    bool updateAsMissing();
    
    // 是否健康（健康即可以输出这个Tracker的预测结果，否则就暂时隐藏这个Tracker的预测结果）
    bool isHealthy();

    size_t id() const { return id_; }
    const TrackerInner &getInner() const { return inner_; }

private:
    void initKalman(const BBox &box);
    cv::Mat stateFromBox(const BBox &box) const;
    BBox boxFromState(const cv::Mat &state) const;

    size_t id_ = 0;
    TrackerInner inner_;
    TrackerConfig cfg_{};

    int life_ = 0;
    int consecutive_hits_ = 0;

    cv::KalmanFilter kf_;
};
