#pragma once

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "config/RoiConfig.h"
#include "structure/LabeledData.h"

// 中文注释：可视化配置，风格上与其他 *Config 结构保持一致
struct VisualizerConfig
{
    int box_thickness = 2;
    int text_thickness = 1;
    double font_scale = 0.6;
    int font_face = cv::FONT_HERSHEY_SIMPLEX;
    int text_padding = 3;
    bool show_score = false;
    bool show_class_id = false;

    // 中文注释：ROI 可视化（用于标记引擎的分析区域）
    RoiConfig roi;
    int roi_thickness = 2;
    cv::Scalar roi_color = cv::Scalar(0, 215, 255); // 金黄色（BGR）
};

class Visualizer
{
public:
    Visualizer();
    explicit Visualizer(VisualizerConfig cfg);

    cv::Mat render(const cv::Mat &frame, const LabeledFrame &data) const;

    void setConfig(const VisualizerConfig &cfg);
    const VisualizerConfig &config() const;

private:
    VisualizerConfig cfg_;

    static cv::Scalar colorForId(int id);
    static cv::Rect clampRect(const cv::Rect &rect, const cv::Size &size);
};
