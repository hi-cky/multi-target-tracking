#include "TrackerManager.h"
#include "matcher/Matcher.h"
#include <vector>

TrackerManager::TrackerManager(const TrackerManagerConfig &cfg) :
    cfg_(cfg), 
    matcher_(std::make_unique<Matcher>(cfg.matcher_cfg)) {}

void TrackerManager::predictAll(float dt) {
    for (auto &t : trackers_) {
        t->predict(dt);
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
    std::vector<TrackerInner> current = currentTrackerInners();
    
    // 2) 将新检测到的 dets 加入到pending_dets中
    addNewDetections(detections);

    // 2) 让当前的tracker和pending_dets匹配
    auto matches = matcher_->match(current, pending_dets_);

    std::vector<char> det_used(pending_dets_.size(), 0);
    for (auto [ti, di] : matches) {
        if (ti < 0 || ti >= static_cast<int>(trackers_.size())) continue;
        if (di < 0 || di >= static_cast<int>(pending_dets_.size())) continue;
        
        // 如果 pending_det 的 age 小于 2，则更新 tracker（避免age为2的状态不新鲜）
        if (pending_dets_[di].age < 2) {
            trackers_[ti]->updateAsHitting(pending_dets_[di]);
        }
        // 然后将其标记为本轮已匹配
        det_used[di] = 1;
        // 同时将其标记为丢弃，避免下一轮再次被匹配到
        // 这里用一个很大的 age 作为“已消费”标记，下一轮会在 addNewDetections 里被过滤掉
        pending_dets_[di].age = 1000000000;
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
    for (size_t d = 0; d < pending_dets_.size(); ++d) {
        if (det_used[d] || pending_dets_[d].age < 2) continue; // 被匹配到的或者是未匹配次数大于等于3次的，才创建一个新的Tracker（延迟匹配）
        trackers_.push_back(std::make_unique<Tracker>(next_id_++, pending_dets_[d], cfg_.tracker_cfg));
    }
    
    // 5) 更新 pending_dets 的age
    for (auto &det : pending_dets_) {
        det.age++;
    }

    return trackers_;
}


std::vector<TrackerInner> TrackerManager::currentTrackerInners() {
    std::vector<TrackerInner> current;
    current.reserve(trackers_.size());
    for (auto &t : trackers_) current.push_back(t->getInner());
    return current;
}


void TrackerManager::addNewDetections(const std::vector<TrackerInner> &detections) {
    // 1) 先和已经存在的 pending_dets 匹配一下
    auto matches = matcher_->match(pending_dets_, detections);
    
    std::vector<TrackerInner> new_pending_dets;
    
    // 2) 记录哪些“新检测”被旧 pending 匹配到了（避免重复加入）
    std::vector<char> det_matched(detections.size(), 0);
    for (const auto &m : matches) {
        // 注意：m.first 对应 pending_dets_ 的索引，m.second 对应 detections 的索引
        if (m.first < 0 || m.first >= static_cast<int>(pending_dets_.size())) continue;
        if (m.second < 0 || m.second >= static_cast<int>(detections.size())) continue;

        det_matched[static_cast<size_t>(m.second)] = 1;

        // 当匹配度高时，用“新的检测信息”覆盖旧 pending 的内容，但 age 不变（实现你说的“替换信息但不重置 age”）
        pending_dets_[static_cast<size_t>(m.first)].box = detections[static_cast<size_t>(m.second)].box;
        pending_dets_[static_cast<size_t>(m.first)].feature = detections[static_cast<size_t>(m.second)].feature;
    }

    new_pending_dets.reserve(pending_dets_.size() + detections.size());

    // 3) 先保留还没过期的旧 pending（其中已匹配的会在上面被更新过内容）
    for (const auto &p : pending_dets_) {
        if (p.age <= 2) {
            new_pending_dets.push_back(p);
        }
    }

    // 4) 再加入未匹配到旧 pending 的新检测，作为新的 pending（age 从 0 开始）
    for (size_t i = 0; i < detections.size(); ++i) {
        if (det_matched[i]) continue;
        TrackerInner fresh = detections[i];
        new_pending_dets.push_back(std::move(fresh));
    }
    
    // 5) 更新 pending_dets_
    pending_dets_.swap(new_pending_dets);
}
