#include "core/processor/StatsRecorder.h"
#include <stdexcept>

StatsRecorder::StatsRecorder(const std::string &path) {
    out_.open(path);
    if (!out_.is_open()) {
        throw std::runtime_error("无法打开输出文件: " + path);
    }
}

void StatsRecorder::consume(const LabeledFrame &data) {
    if (!out_.is_open()) return;
    if (!wrote_header_) {
        out_ << "frame,id,x,y,w,h,class_id,score\n";
        wrote_header_ = true;
    }
    for (const auto &obj : data.objs) {
        out_ << data.frame_index << ','
             << obj.id << ','
             << obj.bbox.x << ','
             << obj.bbox.y << ','
             << obj.bbox.width << ','
             << obj.bbox.height << ','
             << obj.class_id << ','
             << obj.score << "\n";
    }
}

void StatsRecorder::finalize() {
    if (out_.is_open()) {
        out_.flush();
        out_.close();
    }
}
