重要说明


统计数据结构 LabeledFrame、LabeledObject 的定义位于附录 A，请确保 StatsRecorder.cpp 中包含了定义这两个结构的头文件；在示例代码中我用一个占位头文件名，你需要替换为你们实际的头文件路径（如 AppendixA.h 等）。
为了便于编译，test_main.cpp 使用了 OpenCV 的 cv::Rect 和 cv::Point 等类型，请确保项目在编译时链接 OpenCV。
建议在 consume 调用前通过 enableExtraStatistics(true) 预先开启附加统计，否则头部与每帧统计列数将不一致。

如何在项目中集成


将 StatsRecorder.h 和 StatsRecorder.cpp 放到可编译的源代码目录中，并确保在项目中链接 OpenCV（因为 LabeledFrame/LabeledObject 使用 cv::Rect 等类型）。
将 test_main.cpp 作为一个简单自测用例，确保在正式提交前通过实际数据结构进行编译测试。
头文件中的 AppendxA.h（或你们实际的头文件名）需要替换为你们项目中定义 LabeledFrame 和 LabeledObject 的头文件路径。
使用步骤示例
默认写入模式：StatsRecorder rec("stats.csv"); rec.consume(frame); rec.finalize();
启用额外统计：StatsRecorder rec2("stats_extra.csv"); rec2.enableExtraStatistics(true); rec2.consume(frame); rec2.finalize();