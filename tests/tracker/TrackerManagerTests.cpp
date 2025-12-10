#include <gtest/gtest.h>
#include "core/processor/tracker_manager/TrackerManager.h"

TEST(TrackerManagerTests, CreateAndMatch) {
    TrackerManagerConfig cfg;
    cfg.match_threshold = 0.1f;
    cfg.iou_weight = 0.5f;
    cfg.feature_weight = 0.5f;
    cfg.max_life = 5;

    TrackerManager mgr(cfg);

    // 初次输入两条检测，新建两条轨迹
    std::vector<TrackerInner> dets{
        {BBox(cv::Rect2f(0, 0, 10, 10), 0, 0.9f), Feature({1.0f, 0.0f})},
        {BBox(cv::Rect2f(100, 100, 10, 10), 0, 0.8f), Feature({0.0f, 1.0f})}
    };

    mgr.predictAll();
    auto &tracks1 = mgr.update(dets);
    ASSERT_EQ(tracks1.size(), 2u);

    // 轻微移动的检测，再次匹配到原轨迹
    std::vector<TrackerInner> dets2{
        {BBox(cv::Rect2f(2, 2, 10, 10), 0, 0.9f), Feature({0.9f, 0.1f})},
        {BBox(cv::Rect2f(102, 102, 10, 10), 0, 0.8f), Feature({0.1f, 0.9f})}
    };
    mgr.predictAll();
    auto &tracks2 = mgr.update(dets2);
    ASSERT_EQ(tracks2.size(), 2u);
}
