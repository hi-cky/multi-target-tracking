#include <gtest/gtest.h>
#include "core/processor/model/feature_extractor/Feature.h"

TEST(FeatureTests, CosineSimilarity) {
    Feature a({1.0f, 0.0f});
    Feature b({0.0f, 1.0f});
    EXPECT_NEAR(a.cosine_similarity(b), 0.0f, 1e-6f);

    Feature c({1.0f, 1.0f});
    Feature d({1.0f, 1.0f});
    EXPECT_NEAR(c.cosine_similarity(d), 1.0f, 1e-6f);
}

TEST(FeatureTests, AddAndScale) {
    Feature a({1.0f, 2.0f, 3.0f});
    Feature b({4.0f, 5.0f, 6.0f});
    Feature sum = a + b;
    ASSERT_EQ(sum.size(), 3u);
    EXPECT_FLOAT_EQ(sum.values()[0], 5.0f);
    EXPECT_FLOAT_EQ(sum.values()[1], 7.0f);
    EXPECT_FLOAT_EQ(sum.values()[2], 9.0f);

    Feature scaled = a * 2.0f;
    EXPECT_FLOAT_EQ(scaled.values()[0], 2.0f);
    EXPECT_FLOAT_EQ(scaled.values()[1], 4.0f);
    EXPECT_FLOAT_EQ(scaled.values()[2], 6.0f);
}
