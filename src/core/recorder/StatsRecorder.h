#pragma once

#include <fstream>
#include <cstddef>
#include <string>
#include <unordered_set>


#include "structure/LabeledData.h"
#include "RecorderConfig.h"


class StatsRecorder {
public:
// 构造函数：指定配置
StatsRecorder(const RecorderConfig &cfg);


// 析构函数，确保资源释放
~StatsRecorder();
    
    
    // 消费一帧的标注数据，写入 CSV
    void consume(const LabeledFrame &data);
    
    
    // 收尾工作：刷新并关闭输出
    void finalize();
    
    
    // 可选：开启/关闭每帧的额外统计指标（如 unique_ids_seen）
    void enableExtraStatistics(bool enable);
    
    // 获取当前统计快照（用于 UI 实时展示）
    struct StatsSnapshot {
        int frame_index = -1;
        int objects_in_frame = 0;
        std::size_t unique_ids_seen = 0;
        std::size_t total_rows_written = 0;
        bool extra_enabled = false;
        std::string csv_path;
    };
    StatsSnapshot snapshot() const;

private:
    RecorderConfig cfg_;
    std::ofstream out;
    bool wroteHeader = false;
    bool enableExtraStats = false;
    std::unordered_set<int> seenIds;
    std::size_t uniqueIdsSeen = 0;
    int lastFrameIndex = -1;
    int lastObjectsInFrame = 0;
    std::size_t totalRowsWritten = 0;
};
