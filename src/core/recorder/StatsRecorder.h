#pragma once


#include <string>
#include <fstream>
#include <cstddef>
#include <unordered_set>


// 需要包含定义 LabeledFrame/LabeledObject 的头文件，示例中用占位名，请替换为实际头文件
#include "structure/LabeledData.h" // 替换为实际路径


class StatsRecorder {
public:
// 构造函数：指定输出文件路径
StatsRecorder(const std::string &path);


// 析构函数，确保资源释放
~StatsRecorder();


// 消费一帧的标注数据，写入 CSV
void consume(const LabeledFrame &data);


// 收尾工作：刷新并关闭输出
void finalize();


// 可选：开启/关闭每帧的额外统计指标（如 unique_ids_seen）
void enableExtraStatistics(bool enable);

private:
    std::ofstream out;
    bool wroteHeader = false;
    bool enableExtraStats = false;
    std::unordered_set<int> seenIds;
    std::size_t uniqueIdsSeen = 0;
};
