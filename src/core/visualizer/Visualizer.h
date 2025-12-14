#pragma once

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "structure/LabeledData.h"

class Visualizer
{
public:
    struct Options
    {
        int boxThickness = 2;
        int textThickness = 1;
        double fontScale = 0.6;
        int fontFace = cv::FONT_HERSHEY_SIMPLEX;
        int textPadding = 3;
        bool showScore = false;
        bool showClassId = false;
    };

    Visualizer();
    explicit Visualizer(Options options);

    cv::Mat render(const cv::Mat &frame, const LabeledFrame &data) const;

    void setOptions(const Options &options);
    const Options &options() const;

private:
    Options options_;

    static cv::Scalar colorForId(int id);
    static cv::Rect clampRect(const cv::Rect &rect, const cv::Size &size);
};
