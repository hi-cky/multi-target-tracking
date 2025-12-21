#include "ui/MainWindowController.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QStandardPaths>
#include <algorithm>
#include <cmath>

#include <opencv2/imgproc.hpp>

#include "ui/MainWindowView.h"

#include "core/capture/VideoFrameSource.h"

namespace {
// 根据帧率换算定时器间隔（兜底为 30ms）
int calcIntervalMs(double fps) {
    if (fps <= 0.0) return 30;
    const int ms = static_cast<int>(std::round(1000.0 / fps));
    return std::max(1, ms);
}
}  // namespace

MainWindowController::MainWindowController(MainWindowView *view, QObject *parent)
    : QObject(parent), view_(view), cfg_mgr_(defaultConfigPath_().toStdString()) {
    if (!view_) return;

    connect(view_, &MainWindowView::openVideoRequested, this, &MainWindowController::onOpenVideo_);
    connect(view_, &MainWindowView::startRequested, this, &MainWindowController::onStartToggle_);
    connect(view_, &MainWindowView::stopRequested, this, &MainWindowController::onStop_);
    connect(view_, &MainWindowView::sourceTypeChanged, this, &MainWindowController::onSourceTypeChanged_);
    connect(view_, &MainWindowView::cameraIndexChanged, this, &MainWindowController::onCameraIndexChanged_);
    connect(view_, &MainWindowView::sampleFpsChanged, this, &MainWindowController::onSampleFpsChanged_);
    connect(view_, &MainWindowView::trackingToggled, this, &MainWindowController::onTrackingToggled_);

    connect(&timer_, &QTimer::timeout, this, &MainWindowController::onTick_);
    timer_.setInterval(30);

    loadConfig_();
    view_->loadConfig(config_);
    syncVisualizerConfig_();
    updateSourceTitle_();
    updateStartAvailability_();
    view_->setStatusText(QStringLiteral("配置已加载：") + defaultConfigPath_());
}

QString MainWindowController::defaultConfigPath_() const {
    // 优先使用标准配置目录（Linux/WSL: ~/.config/<AppName>/config.yml）
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    const QString appName = QCoreApplication::applicationName().isEmpty()
                                ? QStringLiteral("multi-target-tracking")
                                : QCoreApplication::applicationName();
    const QString baseDir = dir.isEmpty() ? QDir::homePath() + "/.config/" + appName : dir;
    return QDir(baseDir).filePath("config.yml");
}

void MainWindowController::loadConfig_() {
    try {
        config_ = cfg_mgr_.load();

        // 若缺少关键路径，则给一个相对项目结构的默认值，方便第一次启动
        if (config_.engine.detector.ort_env_config.model_path.empty()) {
            config_.engine.detector.ort_env_config.model_path = "model/yolo12n.onnx";
        }
        if (config_.engine.extractor.ort_env_config.model_path.empty()) {
            config_.engine.extractor.ort_env_config.model_path = "model/osnet_x1_0.onnx";
        }
        if (config_.recorder.stats_csv_path.empty()) {
            config_.recorder.stats_csv_path = "docs/output.csv";
        }

        // 确保配置文件存在（首次启动写一份出来，便于用户修改）
        saveConfig_();
    } catch (const std::exception &e) {
        QMessageBox::warning(view_, "配置加载失败", e.what());
    }
}

void MainWindowController::saveConfig_() {
    try {
        const QString path = defaultConfigPath_();
        QDir().mkpath(QFileInfo(path).absolutePath());
        cfg_mgr_.save(config_);
    } catch (const std::exception &e) {
        if (view_) QMessageBox::warning(view_, "配置保存失败", e.what());
    }
}

void MainWindowController::onOpenVideo_() {
    if (!view_) return;
    const QString path = QFileDialog::getOpenFileName(view_, "选择视频文件", QString(), "Video (*.mp4 *.avi *.mov)");
    if (path.isEmpty()) return;

    current_video_path_ = path;
    view_->setVideoPath(path);
    view_->setVideoName(QStringLiteral("当前视频：") + QFileInfo(path).fileName());
    view_->setStatusText(QStringLiteral("已选择视频：") + QFileInfo(path).fileName());
    updateStartAvailability_();
}

bool MainWindowController::startRun_() {
    if (!view_) return false;

    try {
        resetIterators_();
        frame_index_ = 0;
        total_frames_ = -1;
        source_is_live_ = false;

        // 从左侧设置面板读取配置并持久化
        config_ = view_->collectConfig();
        if (config_.engine.detector.ort_env_config.model_path.empty()) {
            config_.engine.detector.ort_env_config.model_path = "model/yolo12n.onnx";
        }
        if (config_.engine.extractor.ort_env_config.model_path.empty()) {
            config_.engine.extractor.ort_env_config.model_path = "model/osnet_x1_0.onnx";
        }
        if (config_.recorder.stats_csv_path.empty()) {
            config_.recorder.stats_csv_path = "docs/output.csv";
        }
        // 可视化 ROI 与引擎 ROI 保持一致
        config_.visualizer.roi = config_.engine.roi;
        saveConfig_();
        view_->loadConfig(config_);
        syncVisualizerConfig_();

        const bool useVideo = view_->isVideoSource();
        const double sampleFps = view_->sampleFps();

        std::unique_ptr<IImageIterator> baseIter;
        if (useVideo) {
            if (current_video_path_.isEmpty()) {
                QMessageBox::information(view_, "提示", "请先选择视频文件");
                return false;
            }
            VideoFrameSource src(current_video_path_.toStdString(), sampleFps);
            baseIter = src.createIterator();
        } else {
            VideoFrameSource src(view_->cameraIndex(), sampleFps);
            baseIter = src.createIterator();
        }

        const FrameSourceInfo info = baseIter->info();
        source_is_live_ = info.is_live;
        total_frames_ = info.total_frames;
        timer_.setInterval(calcIntervalMs(info.sample_fps > 0.0 ? info.sample_fps : info.source_fps));

        if (view_->trackingEnabled()) {
            // 追踪模式走 TrackingEngine
            engine_ = std::make_unique<TrackingEngine>(config_.engine);
            iterator_ = engine_->run(std::move(baseIter));

            if (!config_.recorder.stats_csv_path.empty()) {
                stats_ = std::make_unique<StatsRecorder>(config_.recorder);
            }
        } else {
            // 不追踪时直接输出原帧，仅用可视化模块显示 ROI
            frame_iter_ = std::move(baseIter);
        }

        updateProgressUi_(-1);
        return true;
    } catch (const std::exception &e) {
        QMessageBox::critical(view_, "错误", e.what());
        return false;
    }
}

void MainWindowController::stopRun_(const QString &statusText) {
    if (!view_) return;

    timer_.stop();
    if (stats_) stats_->finalize();

    resetIterators_();
    run_state_ = RunState::Idle;

    view_->setStartButtonText(QStringLiteral("开始"));
    view_->setInputControlsEnabled(true);
    view_->setStopButtonEnabled(false);
    updateStartAvailability_();

    if (!statusText.isEmpty()) {
        view_->setStatusText(statusText);
    }
}

void MainWindowController::resetIterators_() {
    iterator_.reset();
    frame_iter_.reset();
    engine_.reset();
    stats_.reset();
}

void MainWindowController::onStartToggle_() {
    if (!view_) return;

    if (run_state_ == RunState::Idle) {
        if (!startRun_()) return;
        run_state_ = RunState::Running;
        view_->setStartButtonText(QStringLiteral("暂停"));
        view_->setInputControlsEnabled(false);
        view_->setStopButtonEnabled(true);
        timer_.start();
        view_->setStatusText(QStringLiteral("运行中…"));
        return;
    }

    if (run_state_ == RunState::Running) {
        timer_.stop();
        run_state_ = RunState::Paused;
        view_->setStartButtonText(QStringLiteral("继续"));
        view_->setStatusText(QStringLiteral("已暂停"));
        return;
    }

    if (run_state_ == RunState::Paused) {
        timer_.start();
        run_state_ = RunState::Running;
        view_->setStartButtonText(QStringLiteral("暂停"));
        view_->setStatusText(QStringLiteral("运行中…"));
        return;
    }
}

void MainWindowController::onStop_() {
    stopRun_(QStringLiteral("已结束"));
}

void MainWindowController::onTick_() {
    if (!view_) return;

    // 追踪模式（走 iterator_）
    if (iterator_) {
        if (!iterator_->hasNext()) {
            stopRun_(QStringLiteral("已结束"));
            return;
        }

        LabeledFrame lf;
        if (!iterator_->next(lf)) {
            stopRun_(QStringLiteral("已结束"));
            return;
        }

        if (stats_) stats_->consume(lf);

        const cv::Mat &raw = iterator_->getFrame();
        cv::Mat vis = viz_.render(raw, lf);
        showMat_(vis);

        frame_index_ = lf.frame_index + 1;
        updateProgressUi_(lf.frame_index);
        return;
    }

    // 非追踪模式（直接读取原帧）
    if (frame_iter_) {
        if (!frame_iter_->hasNext()) {
            stopRun_(QStringLiteral("已结束"));
            return;
        }

        cv::Mat frame;
        if (!frame_iter_->next(frame)) {
            stopRun_(QStringLiteral("已结束"));
            return;
        }

        LabeledFrame lf;
        lf.frame_index = frame_index_;
        cv::Mat vis = viz_.render(frame, lf);
        showMat_(vis);

        updateProgressUi_(frame_index_);
        ++frame_index_;
    }
}

void MainWindowController::updateProgressUi_(int frameIndex) {
    if (!view_) return;

    const int displayIndex = std::max(0, frameIndex + 1);

    if (source_is_live_) {
        view_->setProgress(0, -1);
        view_->setFrameInfo(QStringLiteral("帧：%1").arg(displayIndex));
        return;
    }

    const int total = total_frames_ > 0 ? total_frames_ : 0;
    view_->setProgress(displayIndex, total);
    if (total > 0) {
        view_->setFrameInfo(QStringLiteral("帧：%1 / %2").arg(displayIndex).arg(total));
    } else {
        view_->setFrameInfo(QStringLiteral("帧：%1").arg(displayIndex));
    }
}

void MainWindowController::showMat_(const cv::Mat &mat) {
    if (!view_) return;
    if (mat.empty()) return;

    cv::Mat rgb;
    cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
    QImage img(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888);
    view_->setPreviewImage(img.copy());  // copy 避免 mat 生命周期影响 UI 显示
}

void MainWindowController::onSourceTypeChanged_() {
    updateSourceTitle_();
    updateStartAvailability_();
}

void MainWindowController::onCameraIndexChanged_(int /*index*/) {
    updateSourceTitle_();
}

void MainWindowController::onSampleFpsChanged_(double /*fps*/) {
    // 采样帧率仅在下次开始时生效
    view_->setStatusText(QStringLiteral("采样帧率已更新（下次开始生效）"));
}

void MainWindowController::onTrackingToggled_(bool /*enabled*/) {
    // 追踪模式切换仅在下次开始时生效
    view_->setStatusText(QStringLiteral("追踪开关已更新（下次开始生效）"));
}

void MainWindowController::updateSourceTitle_() {
    if (!view_) return;

    if (view_->isVideoSource()) {
        if (current_video_path_.isEmpty()) {
            view_->setVideoName(QStringLiteral("当前视频：未选择"));
        } else {
            view_->setVideoName(QStringLiteral("当前视频：") + QFileInfo(current_video_path_).fileName());
        }
        return;
    }

    view_->setVideoName(QStringLiteral("当前摄像头：#%1").arg(view_->cameraIndex()));
}

void MainWindowController::updateStartAvailability_() {
    if (!view_) return;

    if (run_state_ != RunState::Idle) {
        view_->setStartButtonEnabled(true);
        view_->setStopButtonEnabled(true);
        return;
    }

    if (view_->isVideoSource()) {
        view_->setStartButtonEnabled(!current_video_path_.isEmpty());
    } else {
        view_->setStartButtonEnabled(true);
    }
    view_->setStopButtonEnabled(false);
}

void MainWindowController::syncVisualizerConfig_() {
    // 同步 ROI 配置到可视化模块（保证非追踪模式也能看到 ROI）
    VisualizerConfig cfg = config_.visualizer;
    cfg.roi = config_.engine.roi;
    viz_.setConfig(cfg);
}
