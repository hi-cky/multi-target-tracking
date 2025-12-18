#include "TrackingEngine.h"

#include <memory>
#include <vector>
#include <algorithm>
#include "ILabeledDataIterator.h"

#include "model/detector/YoloDetector.h"
#include "model/feature_extractor/FeatureExtractor.h"
#include "tracker_manager/TrackerManager.h"

namespace {
// 中文注释：判断 bbox（像素坐标）中心点是否在 ROI 内
bool centerInRoi(const cv::Rect &bbox, const cv::Rect &roi) {
    if (roi.width <= 0 || roi.height <= 0) return true;
    const float cx = static_cast<float>(bbox.x) + static_cast<float>(bbox.width) * 0.5F;
    const float cy = static_cast<float>(bbox.y) + static_cast<float>(bbox.height) * 0.5F;
    return (cx >= static_cast<float>(roi.x) &&
            cy >= static_cast<float>(roi.y) &&
            cx < static_cast<float>(roi.x + roi.width) &&
            cy < static_cast<float>(roi.y + roi.height));
}

class LabeledDataIteratorImpl : public ILabeledDataIterator {
public:
    LabeledDataIteratorImpl(
        std::unique_ptr<IImageIterator> iter,
        std::unique_ptr<IDetector> detector,
        std::unique_ptr<IFeatureExtractor> extractor,
        std::unique_ptr<TrackerManager> tracker_mgr,
        TrackingEngineConfig cfg
    ): image_iter_(std::move(iter)),
       detector_(std::move(detector)),
       extractor_(std::move(extractor)),
       tracker_mgr_(std::move(tracker_mgr)),
       cfg_(std::move(cfg)) {}

    bool hasNext() const override { return image_iter_ && image_iter_->hasNext(); }

    bool next(LabeledFrame &label) override {
        if (!image_iter_ || !image_iter_->hasNext()) return false;
        if (!image_iter_->next(frame_)) return false;

        // 中文注释：根据当前帧尺寸换算 ROI（固定一次选择，但像素值依赖视频分辨率）
        const cv::Rect roi_px = RoiToPixelRect(cfg_.roi, frame_.size());
        
        // ===卡尔曼滤波根据上一帧的状态对这一帧的结果进行预测===
        // 1) 预测所有轨迹
        tracker_mgr_->predictAll();
        
        // 2) 输出所有traker对于当前这一帧的预测结果（统一由 TrackerManager 负责组装，避免各处重复实现）
        // update 返回值可用于调试/扩展；当前输出直接从 tracker_mgr_ 导出
        tracker_mgr_->fillLabeledFrame(frame_index_, label);

        // 中文注释：ROI 模式下，只输出 ROI 内的标注（bbox 坐标仍是原帧坐标系）
        if (roi_px.area() > 0) {
            auto &objs = label.objs;
            objs.erase(
                std::remove_if(objs.begin(), objs.end(), [&](const LabeledObject &obj) {
                    return !centerInRoi(obj.bbox, roi_px);
                }),
                objs.end()
            );
        }

        
        // ===对当前这一帧进行检测和特征提取，更新跟踪器的状态
        // 3-1) 检测边界框
        // 中文注释：若启用 ROI，则仅对 ROI 子图做检测以减少计算量；
        // 检测结果会在下方加上 (roi.x, roi.y) 偏移映射回原帧坐标系。
        cv::Mat detect_input = frame_;
        const int roi_offset_x = roi_px.area() > 0 ? roi_px.x : 0;
        const int roi_offset_y = roi_px.area() > 0 ? roi_px.y : 0;
        if (roi_px.area() > 0) {
            detect_input = frame_(roi_px);
        }
        auto boxes = detector_->detect(detect_input, frame_index_);

        // 3-2) 为检测框抽特征
        std::vector<TrackerInner> dets;
        dets.reserve(boxes.size());
        for (auto &b : boxes) {
            // 中文注释：将 ROI 局部坐标映射回原帧坐标系（未启用 ROI 时 offset=0）
            b.box.x += static_cast<float>(roi_offset_x);
            b.box.y += static_cast<float>(roi_offset_y);

            // 裁剪区域；若超界则 clip
            cv::Rect2f roi_f = b.box & cv::Rect2f(0, 0, static_cast<float>(frame_.cols), static_cast<float>(frame_.rows));
            if (roi_f.width <= 0 || roi_f.height <= 0) continue;
            cv::Rect roi(cv::Point(static_cast<int>(roi_f.x), static_cast<int>(roi_f.y)),
                         cv::Size(static_cast<int>(roi_f.width), static_cast<int>(roi_f.height)));
            cv::Mat patch = frame_(roi).clone();
            auto feat_vec = extractor_->extract(patch);
            dets.push_back(TrackerInner{b, Feature(std::move(feat_vec))});
        }

        // 4) 更新所有tracker的状态
        tracker_mgr_->update(dets);

        ++frame_index_;
        return true;
    }

    const cv::Mat &getFrame() const override { return frame_; }

private:
    std::unique_ptr<IImageIterator> image_iter_;
    std::unique_ptr<IDetector> detector_;
    std::unique_ptr<IFeatureExtractor> extractor_;
    std::unique_ptr<TrackerManager> tracker_mgr_;
    TrackingEngineConfig cfg_; // [TODO] 这里不需要把整个配置都传进去
    int frame_index_ = 0;
    cv::Mat frame_;
};
}  // namespace

TrackingEngine::TrackingEngine(const TrackingEngineConfig &cfg): cfg_(cfg) {
    detector_ = std::make_unique<YoloDetector>(cfg.detector);
    extractor_ = std::make_unique<FeatureExtractor>(cfg.extractor);
    tracker_mgr_ = std::make_unique<TrackerManager>(cfg.tracker_mgr);
}

std::unique_ptr<ILabeledDataIterator> TrackingEngine::run(std::unique_ptr<IImageIterator> imageIter) {
    return std::make_unique<LabeledDataIteratorImpl>(
        std::move(imageIter),
        std::move(detector_), 
        std::move(extractor_), 
        std::move(tracker_mgr_),
        cfg_
    );
}
