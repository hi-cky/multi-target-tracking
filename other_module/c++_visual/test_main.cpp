#include <iostream>
#include <opencv2/core.hpp>

// 替换为你们实际的头文件路径，以包含 LabeledFrame/LabeledObject 的定义
#include "StatsRecorder.h"
#include "AppendixA.h" // 替换为实际头文件路径

int main()
{
    // 构造一个简单的帧数据：两帧，每帧两个目标
    LabeledFrame frame0;
    frame0.frame_index = 0;
    LabeledObject o1;
    o1.id = 1;
    o1.bbox = cv::Rect(10, 10, 60, 60);
    o1.class_id = 0;
    o1.score = 0.92f;

    LabeledObject o2;
    o2.id = 2;
    o2.bbox = cv::Rect(100, 50, 80, 80);
    o2.class_id = 0;
    o2.score = 0.85f;

    frame0.objs.push_back(o1);
    frame0.objs.push_back(o2);

    LabeledFrame frame1;
    frame1.frame_index = 1;
    LabeledObject o3;
    o3.id = 3;
    o3.bbox = cv::Rect(20, 20, 40, 40);
    o3.class_id = 0;
    o3.score = 0.78f;

    LabeledObject o4;
    o4.id = 1; // 重复 ID，测试统计去重
    o4.bbox = cv::Rect(150, 120, 50, 50);
    o4.class_id = 0;
    o4.score = 0.66f;

    frame1.objs.push_back(o3);
    frame1.objs.push_back(o4);

    // 1) 不启用额外统计的简单用例
    StatsRecorder rec("stats.csv");
    rec.consume(frame0);
    rec.consume(frame1);
    rec.finalize();
    std::cout << "stats.csv 已写入（包含 frame/id/x/y/w/h/class_id/score）" << std::endl;

    // 2) 启用额外统计，写入 unique_ids_seen
    StatsRecorder recExtra("stats_extra.csv");
    recExtra.enableExtraStatistics(true);
    recExtra.consume(frame0);
    recExtra.consume(frame1);
    recExtra.finalize();
    std::cout << "stats_extra.csv 已写入（包含 unique_ids_seen）" << std::endl;

    return 0;
}
