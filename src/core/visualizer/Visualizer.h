#pragma once

#include <opencv2/core.hpp>
#include "structure/LabeledData.h"

class Visualizer {
public:
    Visualizer() = default;
    cv::Mat render(const cv::Mat &frame, const LabeledFrame &data) const;
};
