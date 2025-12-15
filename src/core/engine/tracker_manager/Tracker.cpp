#include "Tracker.h"

#include <algorithm>

Tracker::Tracker(size_t id, const TrackerInner &inner, const TrackerConfig &cfg)
    : id_(id), inner_(inner), cfg_(cfg), life_(cfg.max_life) {
    initKalman(inner.box);
}

void Tracker::initKalman(const BBox &box) {
    // 状态: [px, py, w, h, vx, vy, vw, vh]
    // px, py 使用框底边中心点，使位置与尺寸变化相对解耦
    kf_.init(8, 4, 0, CV_32F);

    // 状态转移矩阵（匀速模型，尺寸也有速度项；vw/vh 增加衰减抑制抖动）
    const float kSizeVelDamping = 0.8f; // 尺寸速度衰减系数，<1 可降低尺寸抖动
    kf_.transitionMatrix = (cv::Mat_<float>(8, 8) <<
        1, 0, 0, 0, 1, 0, 0, 0,
        0, 1, 0, 0, 0, 1, 0, 0,
        0, 0, 1, 0, 0, 0, 1, 0, // w 受 vw 影响
        0, 0, 0, 1, 0, 0, 0, 1, // h 受 vh 影响
        0, 0, 0, 0, 1, 0, 0, 0,
        0, 0, 0, 0, 0, 1, 0, 0,
        0, 0, 0, 0, 0, 0, kSizeVelDamping, 0,
        0, 0, 0, 0, 0, 0, 0, kSizeVelDamping);

    // 观测矩阵：观测 px,py,w,h（不直接观测速度）
    kf_.measurementMatrix = cv::Mat::zeros(4, 8, CV_32F);
    kf_.measurementMatrix.at<float>(0, 0) = 1.0f;
    kf_.measurementMatrix.at<float>(1, 1) = 1.0f;
    kf_.measurementMatrix.at<float>(2, 2) = 1.0f;
    kf_.measurementMatrix.at<float>(3, 3) = 1.0f;

    // 噪声设定：位置平滑，尺寸变化允许更大波动
    kf_.processNoiseCov = cv::Mat::eye(8, 8, CV_32F);
    kf_.processNoiseCov.at<float>(0, 0) = 1e-3f; // px
    kf_.processNoiseCov.at<float>(1, 1) = 1e-3f; // py
    kf_.processNoiseCov.at<float>(2, 2) = 2e-3f; // w 更小过程噪声，平滑尺寸
    kf_.processNoiseCov.at<float>(3, 3) = 2e-3f; // h
    kf_.processNoiseCov.at<float>(4, 4) = 1e-3f; // vx
    kf_.processNoiseCov.at<float>(5, 5) = 1e-3f; // vy
    kf_.processNoiseCov.at<float>(6, 6) = 1e-2f; // vw 保留一定灵敏度
    kf_.processNoiseCov.at<float>(7, 7) = 1e-2f; // vh

    kf_.measurementNoiseCov = cv::Mat::eye(4, 4, CV_32F);
    kf_.measurementNoiseCov.at<float>(0, 0) = 1e-2f; // 位置观测噪声小
    kf_.measurementNoiseCov.at<float>(1, 1) = 1e-2f;
    kf_.measurementNoiseCov.at<float>(2, 2) = 1e-1f; // 尺寸观测噪声更大，减小抖动
    kf_.measurementNoiseCov.at<float>(3, 3) = 1e-1f;

    setIdentity(kf_.errorCovPost, cv::Scalar::all(1));

    kf_.statePost = stateFromBox(box);
}

cv::Mat Tracker::stateFromBox(const BBox &box) const {
    cv::Mat state(8, 1, CV_32F);
    // px/py 取底边中心，使尺寸突变不影响位置
    const float px = box.box.x + box.box.width * 0.5f;
    const float py = box.box.y + box.box.height;
    state.at<float>(0) = px;
    state.at<float>(1) = py;
    state.at<float>(2) = box.box.width;
    state.at<float>(3) = box.box.height;
    state.at<float>(4) = 0.0f; // vx
    state.at<float>(5) = 0.0f; // vy
    state.at<float>(6) = 0.0f; // vw
    state.at<float>(7) = 0.0f; // vh
    return state;
}

BBox Tracker::boxFromState(const cv::Mat &state) const {
    const float px = state.at<float>(0);
    const float py = state.at<float>(1);
    const float w = std::max(1.0f, state.at<float>(2));
    const float h = std::max(1.0f, state.at<float>(3));
    // 由底边中心点恢复左上角坐标
    const float x = px - w * 0.5f;
    const float y = py - h;
    return BBox(cv::Rect2f(x, y, w, h), inner_.box.class_id, inner_.box.score);
}

void Tracker::predict() {
    cv::Mat pred = kf_.predict();
    inner_.box = boxFromState(pred);
}

bool Tracker::updateAsHitting(const TrackerInner &detection) {
    // Kalman 更新
    cv::Mat meas = (cv::Mat_<float>(4, 1) <<
                    detection.box.box.x + detection.box.box.width * 0.5f,
                    detection.box.box.y + detection.box.box.height, // 底边中心 y
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
    return life_ > 0;
    // return life_ == cfg_.max_life; // 改为如果当前是满血则视为健康
}
