#pragma once

#include <memory>

#include "model/detector/IDetector.h"
#include "model/feature_extractor/IFeatureExtractor.h"
#include "tracker_manager/TrackerManager.h"

#include "ILabeledDataIterator.h"
#include "../capture/IImageIterator.h"
#include "config/RoiConfig.h"

struct TrackingEngineConfig {
    DetectorConfig detector;
    FeatureExtractorConfig extractor;
    TrackerManagerConfig tracker_mgr;
    RoiConfig roi;
};

class TrackingEngine {
public:
    TrackingEngine(const TrackingEngineConfig &cfg = {});

    // 输入：图像迭代器；输出：标注数据迭代器
    std::unique_ptr<ILabeledDataIterator> run(std::unique_ptr<IImageIterator> imageIter);

private:
    std::unique_ptr<IDetector> detector_;
    std::unique_ptr<IFeatureExtractor> extractor_;
    std::unique_ptr<TrackerManager> tracker_mgr_;
    TrackingEngineConfig cfg_;
};
