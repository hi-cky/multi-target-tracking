#pragma once

#include <string>

#include <opencv2/core.hpp>

#include "core/processor/model/detector/IDetector.h"
#include "core/processor/model/feature_extractor/IFeatureExtractor.h"
#include "core/processor/tracker_manager/TrackerManager.h"

// 中文注释：统一的应用配置（用于持久化到 config.yml）
// 设计目标：
// 1) 结构化：与代码里的 Config 结构一一对应，避免散落的 key/value
// 2) 可扩展：未来 UI 布局/交互配置也可以加在这里
// 3) 易迁移：缺字段时使用默认值，保证向后兼容
struct AppConfig {
    DetectorConfig detector;
    FeatureExtractorConfig feature;
    TrackerManagerConfig tracker_mgr;

    // 中文注释：统计/日志输出（可选）
    std::string stats_csv_path = {};

    // 中文注释：从 YAML 文件加载；若文件不存在/字段缺失，会尽量保留默认值
    static AppConfig loadFromFile(const std::string &path);

    // 中文注释：保存到 YAML 文件（覆盖写入）
    void saveToFile(const std::string &path) const;
};
