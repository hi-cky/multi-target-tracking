#pragma once

#include <fstream>
#include <string>
#include "structure/LabeledData.h"

class StatsRecorder {
public:
    explicit StatsRecorder(const std::string &path);
    void consume(const LabeledFrame &data);
    void finalize();
private:
    std::ofstream out_;
    bool wrote_header_ = false;
};
