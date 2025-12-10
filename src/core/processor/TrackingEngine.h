#pragma once

#include <memory>

#include "model/detector/IDetector.h"
#include "model/feature_extractor/IFeatureExtractor.h"
#include "tracker_manager/TrackerManager.h"
#include "ILabeledDataIterator.h"
#include "../capture/IImageIterator.h"

class TrackingEngine {
public:
    struct Config {
        DetectorConfig detector;
        FeatureExtractorConfig feature;
        TrackerManagerConfig tracker_mgr;
    };

    TrackingEngine(std::unique_ptr<IDetector> detector,
                   std::unique_ptr<IFeatureExtractor> extractor,
                   const Config &cfg = {});

    // 输入：图像迭代器；输出：标注数据迭代器
    std::unique_ptr<ILabeledDataIterator> run(std::unique_ptr<IImageIterator> imageIter);

private:
    std::unique_ptr<IDetector> detector_;
    std::unique_ptr<IFeatureExtractor> extractor_;
    Config cfg_;
};
