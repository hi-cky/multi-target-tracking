#include "core/visualizer/Visualizer.h"

#include <opencv2/imgproc.hpp>
#include <string>

cv::Mat Visualizer::render(const cv::Mat &frame, const LabeledFrame &data) const {
    cv::Mat out = frame.clone();
    for (const auto &obj : data.objs) {
        cv::rectangle(out, obj.bbox, cv::Scalar(0, 255, 0), 2);
        std::string text = "ID:" + std::to_string(obj.id);
        cv::putText(out, text, cv::Point(obj.bbox.x, std::max(0, obj.bbox.y - 5)),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 1);
    }
    return out;
}
