#include "StatsRecorder.h"
#include <stdexcept>
#include <vector>

// 注意：以下包含需要替换为你们实际项目中定义 LabeledFrame/LabeledObject 的头文件
// #include "AppendixA.h" // 替换为实际头文件路径

StatsRecorder::StatsRecorder(const RecorderConfig &cfg)
    : cfg_(cfg), wroteHeader(false), enableExtraStats(cfg.enable_extra_statistics), uniqueIdsSeen(0)
{
    out.open(cfg.stats_csv_path.c_str(), std::ofstream::out | std::ofstream::trunc);
    if (!out.is_open())
    {
        throw std::runtime_error("Cannot open stats file: " + cfg.stats_csv_path);
    }
}

StatsRecorder::~StatsRecorder()
{
    finalize();
}

void StatsRecorder::enableExtraStatistics(bool enable)
{
    enableExtraStats = enable;
    // 注意：如果已经写过头部，开启后无法自动重新写头部以对齐新字段，
    // 因此请确保在 consume 之前调用此方法。若需要动态切换，请在此处实现头部重写逻辑。
}

void StatsRecorder::consume(const LabeledFrame &data)
{
    if (!out.is_open())
        return;

    // 更新已出现的唯一ID集合（在写入之前更新，以确保 uniqueIdsSeen 为当前帧之后的总唯一数）
    for (const LabeledObject &obj : data.objs)
    {
        auto ret = seenIds.insert(obj.id);
        if (ret.second)
        {
            uniqueIdsSeen = seenIds.size();
        }
    }

    // 写入表头（根据是否开启额外统计动态决定列名）
    if (!wroteHeader)
    {
        if (enableExtraStats)
        {
            out << "frame,id,x,y,w,h,class_id,score,unique_ids_seen\n";
        }
        else
        {
            out << "frame,id,x,y,w,h,class_id,score\n";
        }
        wroteHeader = true;
    }

    // 按目标逐行写入当前帧的目标数据
    for (size_t i = 0; i < data.objs.size(); ++i)
    {
        const LabeledObject &obj = data.objs[i];
        out << data.frame_index << ","
            << obj.id << ","
            << obj.bbox.x << ","
            << obj.bbox.y << ","
            << obj.bbox.width << ","
            << obj.bbox.height << ","
            << obj.class_id << ","
            << obj.score;

        if (enableExtraStats)
        {
            out << "," << uniqueIdsSeen;
        }

        out << "\n";
    }
}

void StatsRecorder::finalize()
{
    if (!out.is_open())
        return;

    out.flush();
    out.close();
}
