#include "TrackingEngine.h"

#include <vector>
#include "ILabeledDataIterator.h"

namespace {
class LabeledDataIteratorImpl : public ILabeledDataIterator {
public:
    LabeledDataIteratorImpl(std::unique_ptr<IImageIterator> iter,
                            TrackingEngine::Config cfg,
                            std::unique_ptr<IDetector> detector,
                            std::unique_ptr<IFeatureExtractor> extractor)
        : image_iter_(std::move(iter)),
          cfg_(cfg),
          detector_(std::move(detector)),
          extractor_(std::move(extractor)),
          tracker_mgr_(cfg.tracker_mgr) {}

    bool hasNext() const override { return image_iter_ && image_iter_->hasNext(); }

    bool next(LabeledFrame &outFrame) override {
        if (!image_iter_ || !image_iter_->hasNext()) return false;
        if (!image_iter_->next(last_frame_)) return false;

        // 1) 预测所有轨迹
        tracker_mgr_.predictAll();

        // 2) 检测
        auto boxes = detector_->detect(last_frame_, frame_index_);

        // 3) 为检测框抽特征
        std::vector<TrackerInner> dets;
        dets.reserve(boxes.size());
        for (auto &b : boxes) {
            // 裁剪区域；若超界则 clip
            cv::Rect2f roi_f = b.box & cv::Rect2f(0, 0, static_cast<float>(last_frame_.cols), static_cast<float>(last_frame_.rows));
            if (roi_f.width <= 0 || roi_f.height <= 0) continue;
            cv::Rect roi(cv::Point(static_cast<int>(roi_f.x), static_cast<int>(roi_f.y)),
                         cv::Size(static_cast<int>(roi_f.width), static_cast<int>(roi_f.height)));
            cv::Mat patch = last_frame_(roi).clone();
            auto feat_vec = extractor_->extract(patch);
            dets.push_back(TrackerInner{b, Feature(std::move(feat_vec))});
        }

        // 4) 更新轨迹
        tracker_mgr_.update(dets);

        // 5) 输出当前帧的标注（统一由 TrackerManager 负责组装，避免各处重复实现）
        // update 返回值可用于调试/扩展；当前输出直接从 tracker_mgr_ 导出
        tracker_mgr_.fillLabeledFrame(frame_index_, outFrame);

        ++frame_index_;
        return true;
    }

    const cv::Mat &lastFrame() const override { return last_frame_; }

private:
    std::unique_ptr<IImageIterator> image_iter_;
    TrackingEngine::Config cfg_;
    std::unique_ptr<IDetector> detector_;
    std::unique_ptr<IFeatureExtractor> extractor_;
    TrackerManager tracker_mgr_;
    int frame_index_ = 0;
    cv::Mat last_frame_;
};
}  // namespace

TrackingEngine::TrackingEngine(std::unique_ptr<IDetector> detector,
                               std::unique_ptr<IFeatureExtractor> extractor,
                               const TrackingEngine::Config &cfg)
    : detector_(std::move(detector)), extractor_(std::move(extractor)), cfg_(cfg) {}

std::unique_ptr<ILabeledDataIterator> TrackingEngine::run(std::unique_ptr<IImageIterator> imageIter) {
    return std::make_unique<LabeledDataIteratorImpl>(std::move(imageIter), cfg_, std::move(detector_), std::move(extractor_));
}
