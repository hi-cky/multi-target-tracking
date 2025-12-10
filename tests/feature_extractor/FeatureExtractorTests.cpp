#include <gtest/gtest.h>
#include <filesystem>
#include <numeric>

#include <opencv2/opencv.hpp>

#include "core/processor/model/feature_extractor/IFeatureExtractor.h"

TEST(FeatureExtractorTests, ExtractFeatureOnBlank) {
    constexpr const char *kProjectRoot = PROJECT_ROOT_DIR;
    const std::filesystem::path model_path = std::filesystem::path(kProjectRoot) / "model" / "osnet_x1_0.onnx";
    ASSERT_TRUE(std::filesystem::exists(model_path));

    FeatureExtractorConfig cfg;
    cfg.model_path = model_path.string();
    cfg.input_height = 256;
    cfg.input_width = 128;

    auto extractor = CreateFeatureExtractor(cfg);

    cv::Mat img(256, 128, CV_8UC3, cv::Scalar(0, 0, 0));
    const auto feat = extractor->extract(img);

    EXPECT_GT(feat.size(), 0U);
    float norm = std::sqrt(std::inner_product(feat.begin(), feat.end(), feat.begin(), 0.0f));
    EXPECT_NEAR(norm, 1.0f, 1e-3);
}
