#include <iostream>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "AppendixA.h"
#include "Visualizer.h"

int main()
{
    const int w = 640;
    const int h = 360;
    cv::Mat frame(h, w, CV_8UC3, cv::Scalar(30, 30, 30));

    LabeledFrame labeled;
    labeled.frame_index = 0;
    labeled.objs.push_back(LabeledObject{1, cv::Rect(30, 40, 120, 200), 0, 0.92f});
    labeled.objs.push_back(LabeledObject{2, cv::Rect(220, 80, 160, 140), 0, 0.85f});
    labeled.objs.push_back(LabeledObject{42, cv::Rect(430, 60, 150, 220), 0, 0.78f});

    Visualizer::Options opts;
    opts.showScore = true;
    Visualizer viz(opts);

    const cv::Mat out = viz.render(frame, labeled);
    if (out.empty())
    {
        std::cerr << "Visualizer output is empty\n";
        return 1;
    }

    cv::Mat diff;
    cv::absdiff(frame, out, diff);
    const cv::Scalar s = cv::sum(diff);
    const double changed = s[0] + s[1] + s[2] + s[3];
    if (changed <= 0.0)
    {
        std::cerr << "Visualizer output did not change the frame\n";
        return 2;
    }

    const std::string outPath = "visualizer_test_output.png";
    if (!cv::imwrite(outPath, out))
    {
        std::cerr << "Failed to write " << outPath << "\n";
        return 3;
    }

    std::cout << "Wrote " << outPath << "\n";
    return 0;
}

