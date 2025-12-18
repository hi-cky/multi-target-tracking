#include "Visualizer.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <string>

#include <opencv2/imgproc.hpp>

Visualizer::Visualizer() : cfg_() {}

Visualizer::Visualizer(VisualizerConfig cfg) : cfg_(std::move(cfg)) {}

void Visualizer::setConfig(const VisualizerConfig &cfg)
{
    cfg_ = cfg;
}

const VisualizerConfig &Visualizer::config() const
{
    return cfg_;
}

static cv::Scalar hsvToBgr(float h, float s, float v)
{
    h = std::fmod(std::max(0.0f, h), 360.0f);
    s = std::clamp(s, 0.0f, 1.0f);
    v = std::clamp(v, 0.0f, 1.0f);

    const float c = v * s;
    const float x = c * (1.0f - std::fabs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
    const float m = v - c;

    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;

    if (h < 60.0f)
    {
        r = c;
        g = x;
        b = 0.0f;
    }
    else if (h < 120.0f)
    {
        r = x;
        g = c;
        b = 0.0f;
    }
    else if (h < 180.0f)
    {
        r = 0.0f;
        g = c;
        b = x;
    }
    else if (h < 240.0f)
    {
        r = 0.0f;
        g = x;
        b = c;
    }
    else if (h < 300.0f)
    {
        r = x;
        g = 0.0f;
        b = c;
    }
    else
    {
        r = c;
        g = 0.0f;
        b = x;
    }

    const auto to255 = [](float f) -> double { return static_cast<double>(std::clamp(f, 0.0f, 1.0f) * 255.0f); };
    return cv::Scalar(to255(b + m), to255(g + m), to255(r + m));
}

cv::Scalar Visualizer::colorForId(int id)
{
    // Deterministic color per ID (Knuth multiplicative hash).
    const std::uint32_t x = static_cast<std::uint32_t>(id) * 2654435761u;
    const float hue = static_cast<float>(x % 360u);
    return hsvToBgr(hue, 0.85f, 0.95f);
}

cv::Rect Visualizer::clampRect(const cv::Rect &rect, const cv::Size &size)
{
    if (size.width <= 0 || size.height <= 0)
        return cv::Rect();

    const int x1 = std::clamp(rect.x, 0, std::max(0, size.width - 1));
    const int y1 = std::clamp(rect.y, 0, std::max(0, size.height - 1));
    const int x2 = std::clamp(rect.x + rect.width, 0, size.width);
    const int y2 = std::clamp(rect.y + rect.height, 0, size.height);
    return cv::Rect(cv::Point(x1, y1), cv::Point(x2, y2));
}

cv::Mat Visualizer::render(const cv::Mat &frame, const LabeledFrame &data) const
{
    if (frame.empty())
    {
        return cv::Mat();
    }

    cv::Mat output;
    if (frame.channels() == 3)
    {
        output = frame.clone();
    }
    else if (frame.channels() == 1)
    {
        cv::cvtColor(frame, output, cv::COLOR_GRAY2BGR);
    }
    else if (frame.channels() == 4)
    {
        cv::cvtColor(frame, output, cv::COLOR_BGRA2BGR);
    }
    else
    {
        output = frame.clone();
    }

    const int boxThickness = std::max(1, cfg_.box_thickness);
    const int textThickness = std::max(1, cfg_.text_thickness);
    const int pad = std::max(0, cfg_.text_padding);

    // 中文注释：绘制 ROI 框（可选），用于直观展示引擎正在分析的子区域
    if (cfg_.roi.enabled)
    {
        const cv::Rect roi = RoiToPixelRect(cfg_.roi, output.size());
        if (roi.width > 0 && roi.height > 0)
        {
            const int roiThickness = std::max(1, cfg_.roi_thickness);
            cv::rectangle(output, roi, cfg_.roi_color, roiThickness);
            cv::putText(
                output,
                "ROI",
                cv::Point(roi.x + 5, std::max(15, roi.y + 15)),
                cfg_.font_face,
                cfg_.font_scale,
                cfg_.roi_color,
                textThickness,
                cv::LINE_AA);
        }
    }

    for (const LabeledObject &obj : data.objs)
    {
        const cv::Scalar color = colorForId(obj.id);
        const cv::Rect bbox = clampRect(obj.bbox, output.size());

        if (bbox.width <= 0 || bbox.height <= 0)
            continue;

        cv::rectangle(output, bbox, color, boxThickness);

        std::string label = "ID:" + std::to_string(obj.id);
        if (cfg_.show_class_id)
        {
            label += " C:" + std::to_string(obj.class_id);
        }
        if (cfg_.show_score)
        {
            char buf[32];
            std::snprintf(buf, sizeof(buf), " S:%.2f", obj.score);
            label += buf;
        }

        int baseline = 0;
        const cv::Size textSize = cv::getTextSize(label, cfg_.font_face, cfg_.font_scale, textThickness, &baseline);

        cv::Point textOrg(bbox.x, bbox.y - 5);
        if (textOrg.y - textSize.height - baseline - pad * 2 < 0)
        {
            textOrg.y = bbox.y + textSize.height + baseline + pad * 2;
        }

        const cv::Rect bgRect(
            textOrg.x,
            textOrg.y - textSize.height - baseline - pad,
            textSize.width + pad * 2,
            textSize.height + baseline + pad * 2);

        const cv::Rect bg = clampRect(bgRect, output.size());
        if (bg.width > 0 && bg.height > 0)
        {
            cv::rectangle(output, bg, cv::Scalar(0, 0, 0), cv::FILLED);
            cv::rectangle(output, bg, color, 1);
        }

        const cv::Point textPoint(bg.x + pad, bg.y + bg.height - baseline - pad);
        cv::putText(output, label, textPoint, cfg_.font_face, cfg_.font_scale, cv::Scalar(255, 255, 255), textThickness, cv::LINE_AA);
    }

    return output;
}
