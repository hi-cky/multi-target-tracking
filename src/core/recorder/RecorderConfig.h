#pragma once

#include <string>

// 中文注释：录制/统计模块配置
struct RecorderConfig {
    // 统计 CSV 输出路径；为空表示不记录
    std::string stats_csv_path = {};
    // 是否输出额外统计（unique_ids_seen 等）
    bool enable_extra_statistics = true;
};

