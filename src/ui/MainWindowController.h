#pragma once

#include <QObject>
#include <QTimer>
#include <QString>
#include <memory>

#include "config/AppConfig.h"
#include "config/ConfigManager.h"
#include "core/engine/TrackingEngine.h"
#include "core/visualizer/Visualizer.h"
#include "core/recorder/StatsRecorder.h"
#include "core/engine/ILabeledDataIterator.h"

class MainWindowView;

// 中文注释：MainWindowController 负责交互逻辑（打开视频/启动/定时刷新/配置读写），不包含布局代码
class MainWindowController final : public QObject {
    Q_OBJECT
public:
    explicit MainWindowController(MainWindowView *view, QObject *parent = nullptr);

private slots:
    void onOpenVideo_();
    void onStart_();
    void onTick_();
    void onSettings_();

private:
    QString defaultConfigPath_() const;
    void loadConfig_();
    void saveConfig_();

    void resetEngine_(const QString &videoPath);
    void showMat_(const cv::Mat &mat);

    MainWindowView *view_ = nullptr;
    QTimer timer_;

    ConfigManager cfg_mgr_;
    AppConfig config_;
    QString current_video_path_;

    std::unique_ptr<TrackingEngine> engine_;
    std::unique_ptr<ILabeledDataIterator> iterator_;
    Visualizer viz_;
    std::unique_ptr<StatsRecorder> stats_;
    int frame_index_ = 0;
};
