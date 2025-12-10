#include <gtest/gtest.h>
#include "core/processor/matcher/Matcher.h"

// 简单贪心匹配：IoU 和特征均有高相似度时应匹配到同类
TEST(MatcherTests, GreedyWeightedMatch) {
    TrackerInner l0{BBox(cv::Rect2f(0, 0, 10, 10), 0, 0.9f), Feature({1.0f, 0.0f})};
    TrackerInner l1{BBox(cv::Rect2f(100, 100, 10, 10), 0, 0.8f), Feature({0.0f, 1.0f})};
    TrackerInner r0{BBox(cv::Rect2f(1, 1, 10, 10), 0, 0.7f), Feature({0.9f, 0.1f})};
    TrackerInner r1{BBox(cv::Rect2f(110, 110, 10, 10), 0, 0.6f), Feature({0.1f, 0.9f})};

    std::vector<TrackerInner> left{l0, l1};
    std::vector<TrackerInner> right{r0, r1};

    auto matcher = CreateMatcher(0.5f, 0.5f, 0.1f);
    auto matches = matcher->match(left, right);

    ASSERT_EQ(matches.size(), 2u);
    EXPECT_TRUE((matches[0] == std::pair<int,int>(0,0) && matches[1] == std::pair<int,int>(1,1)) ||
                (matches[0] == std::pair<int,int>(1,1) && matches[1] == std::pair<int,int>(0,0)));
}
