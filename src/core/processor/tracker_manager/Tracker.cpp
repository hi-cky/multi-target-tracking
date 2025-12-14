#include "Tracker.h"

#include <algorithm>

Tracker::Tracker(size_t id, const TrackerInner &inner, const TrackerConfig &cfg)
    : id_(id), inner_(inner), cfg_(cfg), life_(cfg.max_life) {
    initKalman(inner.box);
}

void Tracker::initKalman(const BBox &box) {
    // 状态: [cx, cy, w, h, vx, vy]
    kf_.init(6, 4, 0, CV_32F);

    // 状态转移矩阵
    kf_.transitionMatrix = (cv::Mat_<float>(6, 6) <<
        1, 0, 0, 0, 1, 0,
        0, 1, 0, 0, 0, 1,
        0, 0, 1, 0, 0, 0,
        0, 0, 0, 1, 0, 0,
        0, 0, 0, 0, 1, 0,
        0, 0, 0, 0, 0, 1);

    // 观测矩阵：观测 cx,cy,w,h
    kf_.measurementMatrix = cv::Mat::zeros(4, 6, CV_32F);
    kf_.measurementMatrix.at<float>(0, 0) = 1.0f;
    kf_.measurementMatrix.at<float>(1, 1) = 1.0f;
    kf_.measurementMatrix.at<float>(2, 2) = 1.0f;
    kf_.measurementMatrix.at<float>(3, 3) = 1.0f;

    setIdentity(kf_.processNoiseCov, cv::Scalar::all(1e-2));
    setIdentity(kf_.measurementNoiseCov, cv::Scalar::all(1e-1));
    setIdentity(kf_.errorCovPost, cv::Scalar::all(1));

    kf_.statePost = stateFromBox(box);
}

cv::Mat Tracker::stateFromBox(const BBox &box) const {
    cv::Mat state(6, 1, CV_32F);
    const float cx = box.box.x + box.box.width * 0.5f;
    const float cy = box.box.y + box.box.height * 0.5f;
    state.at<float>(0) = cx;
    state.at<float>(1) = cy;
    state.at<float>(2) = box.box.width;
    state.at<float>(3) = box.box.height;
    state.at<float>(4) = 0.0f; // vx
    state.at<float>(5) = 0.0f; // vy
    return state;
}

BBox Tracker::boxFromState(const cv::Mat &state) const {
    const float cx = state.at<float>(0);
    const float cy = state.at<float>(1);
    const float w = std::max(1.0f, state.at<float>(2));
    const float h = std::max(1.0f, state.at<float>(3));
    const float x = cx - w * 0.5f;
    const float y = cy - h * 0.5f;
    return BBox(cv::Rect2f(x, y, w, h), inner_.box.class_id, inner_.box.score);
}

void Tracker::predict() {
    cv::Mat pred = kf_.predict();
    inner_.box = boxFromState(pred);
    // 预测阶段不修改 life/consecutive_misses
}

bool Tracker::updateAsHitting(const TrackerInner &detection) {
    // Kalman 更新
    cv::Mat meas = (cv::Mat_<float>(4, 1) <<
                    detection.box.box.x + detection.box.box.width * 0.5f,
                    detection.box.box.y + detection.box.box.height * 0.5f,
                    detection.box.box.width,
                    detection.box.box.height);
    kf_.correct(meas);
    inner_.box = detection.box;

    // 特征滑动平均
    std::vector<float> fused;
    fused.resize(inner_.feature.size());
    const auto &old_f = inner_.feature.values();
    const auto &new_f = detection.feature.values();
    const float alpha = cfg_.feature_momentum;
    for (size_t i = 0; i < fused.size(); ++i) {
        fused[i] = alpha * new_f[i] + (1.0f - alpha) * old_f[i];
    }
    inner_.feature = Feature(std::move(fused)).normalized();

    consecutive_hits_ = std::min(3, consecutive_hits_ + 1);
    life_ = std::min(cfg_.max_life, life_ + (1 << consecutive_hits_));
    return true;
}

bool Tracker::updateAsMissing() {
    consecutive_hits_ = 0;
    life_ = std::max(0, life_ - 1);
    return life_ == 0;
}


bool Tracker::isHealthy() {
    // return life_ > 0;
    return life_ == cfg_.max_life; // 改为如果当前是满血则视为健康
}
