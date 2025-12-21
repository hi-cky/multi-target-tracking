#pragma once

#include <QObject>
#include <QTimer>
#include <QString>
#include <memory>

#include "config/AppConfig.h"
#include "config/ConfigManager.h"
#include "core/engine/TrackingEngine.h"
#include "core/capture/IImageIterator.h"
#include "core/visualizer/Visualizer.h"
#include "core/recorder/StatsRecorder.h"
#include "core/engine/ILabeledDataIterator.h"

class MainWindowView;

// MainWindowController 负责交互逻辑（打开视频/启动/定时刷新/配置读写），不包含布局代码
class MainWindowController final : public QObject {
    Q_OBJECT
public:
    explicit MainWindowController(MainWindowView *view, QObject *parent = nullptr);

private slots:
    void onOpenVideo_();
    void onStartToggle_();
    void onStop_();
    void onTick_();
    void onSourceTypeChanged_();
    void onCameraIndexChanged_(int index);
    void onSampleFpsChanged_(double fps);
    void onTrackingToggled_(bool enabled);

private:
    QString defaultConfigPath_() const;
    void loadConfig_();
    void saveConfig_();

    bool startRun_();
    void stopRun_(const QString &statusText);
    void resetIterators_();
    void updateSourceTitle_();
    void updateStartAvailability_();
    void updateProgressUi_(int frameIndex);
    void syncVisualizerConfig_();
    void showMat_(const cv::Mat &mat);

    MainWindowView *view_ = nullptr;
    QTimer timer_;

    ConfigManager cfg_mgr_;
    AppConfig config_;
    QString current_video_path_;

    std::unique_ptr<TrackingEngine> engine_;
    std::unique_ptr<ILabeledDataIterator> iterator_;
    std::unique_ptr<IImageIterator> frame_iter_;
    Visualizer viz_;
    std::unique_ptr<StatsRecorder> stats_;
    int frame_index_ = 0;
    int total_frames_ = -1;
    bool source_is_live_ = false;

    enum class RunState { Idle, Running, Paused };
    RunState run_state_ = RunState::Idle;
};
