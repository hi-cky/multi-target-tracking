#include "TrackerManager.h"
#include "matcher/Matcher.h"

TrackerManager::TrackerManager(const TrackerManagerConfig &cfg) :
    cfg_(cfg), 
    matcher_(std::make_unique<Matcher>(cfg.matcher_cfg)) {}

void TrackerManager::predictAll() {
    for (auto &t : trackers_) {
        t->predict();
    }
}

void TrackerManager::fillLabeledFrame(int frame_index, LabeledFrame &label) const {
    // 将当前健康的 trackers_ 导出为统一的标注结构，供 UI/下游模块使用
    label.frame_index = frame_index;
    label.objs.clear();
    label.objs.reserve(trackers_.size());

    for (const auto &t : trackers_) {
        if (!t->isHealthy()) continue;
        const auto &inner = t->getInner();

        LabeledObject obj;
        obj.id = static_cast<int>(t->id());
        obj.bbox = inner.box.box;  // cv::Rect2f -> cv::Rect（OpenCV 支持转换/截断）
        obj.class_id = inner.box.class_id;
        obj.score = inner.box.score;
        label.objs.push_back(obj);
    }
}

const std::vector<std::unique_ptr<Tracker>> &
TrackerManager::update(const std::vector<TrackerInner> &detections) {
    // 1) 预测阶段已在外部或通过 predictAll 调用，这里直接拿当前 tracker inner
    std::vector<TrackerInner> current;
    current.reserve(trackers_.size());
    for (auto &t : trackers_) current.push_back(t->getInner());

    // 2) 匹配
    auto matches = matcher_->match(current, detections);

    std::vector<char> det_used(detections.size(), 0);
    for (auto [ti, di] : matches) {
        if (ti < 0 || ti >= static_cast<int>(trackers_.size())) continue;
        trackers_[ti]->updateAsHitting(detections[di]);
        det_used[di] = 1;
    }

    // 3) 对未匹配的 tracker 做 missed 更新，标记清除
    std::vector<std::unique_ptr<Tracker>> alive;
    alive.reserve(trackers_.size());
    for (size_t i = 0; i < trackers_.size(); ++i) {
        bool removed = false;
        // 若该 tracker 未被命中
        bool hit = false;
        for (auto &m : matches) {
            if (static_cast<size_t>(m.first) == i) { hit = true; break; }
        }
        if (!hit) {
            removed = trackers_[i]->updateAsMissing();
        }
        if (!removed) {
            alive.push_back(std::move(trackers_[i]));
        }
    }
    trackers_.swap(alive);

    // 4) 为未匹配的检测创建新 tracker
    for (size_t d = 0; d < detections.size(); ++d) {
        if (det_used[d]) continue;
        trackers_.push_back(std::make_unique<Tracker>(next_id_++, detections[d], cfg_.tracker_cfg));
    }

    return trackers_;
}
