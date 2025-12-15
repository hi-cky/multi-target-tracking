#pragma once

#include <string>

#include <opencv2/core.hpp>

#include "core/engine/TrackingEngine.h"
#include "core/recorder/RecorderConfig.h"

class ConfigManager;

// 中文注释：统一的应用配置（用于持久化到 config.yml）
// 设计目标：
// 1) 结构化：与业务对象一一对应，避免散乱 key/value
// 2) 可扩展：未来 UI/日志/新模块的配置可继续嵌套
// 3) 易迁移：缺字段时使用默认值，保证向后兼容
struct AppConfig {
    // 中文注释：Engine 全量配置（包含 detector / extractor / tracker）
    TrackingEngineConfig engine;

    // 中文注释：录制/统计模块配置
    RecorderConfig recorder;

    // 中文注释：从 YAML 文件加载；若文件不存在/字段缺失，会尽量保留默认值
    static AppConfig loadFromFile(const std::string &path);

    // 中文注释：保存到 YAML 文件（覆盖写入）
    void saveToFile(const std::string &path) const;
};
