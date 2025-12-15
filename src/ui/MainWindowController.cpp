#include "ui/MainWindowController.h"

#include <QCoreApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>

#include <opencv2/imgproc.hpp>

#include "ui/MainWindowView.h"
#include "ui/ConfigDialog.h"

#include "core/capture/VideoFrameSource.h"

MainWindowController::MainWindowController(MainWindowView *view, QObject *parent)
    : QObject(parent), view_(view), cfg_mgr_(defaultConfigPath_().toStdString()) {
    if (!view_) return;

    connect(view_, &MainWindowView::openVideoRequested, this, &MainWindowController::onOpenVideo_);
    connect(view_, &MainWindowView::startRequested, this, &MainWindowController::onStart_);
    connect(view_, &MainWindowView::settingsRequested, this, &MainWindowController::onSettings_);

    connect(&timer_, &QTimer::timeout, this, &MainWindowController::onTick_);
    timer_.setInterval(30);

    loadConfig_();
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
        const QString path = defaultConfigPath_();
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
    resetEngine_(path);
}

void MainWindowController::resetEngine_(const QString &videoPath) {
    if (!view_) return;

    try {
        // 如正在运行则先停止
        timer_.stop();
        iterator_.reset();
        engine_.reset();
        stats_.reset();
        frame_index_ = 0;

        engine_ = std::make_unique<TrackingEngine>(config_.engine);

        VideoFrameSource src(videoPath.toStdString());
        iterator_ = engine_->run(src.createIterator());

        if (!config_.recorder.stats_csv_path.empty()) {
            stats_ = std::make_unique<StatsRecorder>(config_.recorder);
        }

        view_->setStatusText(QStringLiteral("已加载视频，点击开始 ▶"));
    } catch (const std::exception &e) {
        QMessageBox::critical(view_, "错误", e.what());
    }
}

void MainWindowController::onStart_() {
    if (!view_) return;
    if (!iterator_) {
        QMessageBox::information(view_, "提示", "请先选择视频文件");
        return;
    }
    timer_.start();
    view_->setStatusText(QStringLiteral("运行中…"));
}

void MainWindowController::onTick_() {
    if (!view_) return;
    if (!iterator_ || !iterator_->hasNext()) {
        timer_.stop();
        if (stats_) stats_->finalize();
        view_->setStatusText(QStringLiteral("已结束"));
        return;
    }

    LabeledFrame lf;
    if (!iterator_->next(lf)) {
        timer_.stop();
        if (stats_) stats_->finalize();
        view_->setStatusText(QStringLiteral("已结束"));
        return;
    }

    if (stats_) stats_->consume(lf);

    const cv::Mat &raw = iterator_->lastFrame();
    cv::Mat vis = viz_.render(raw, lf);
    showMat_(vis);
    ++frame_index_;
}

void MainWindowController::showMat_(const cv::Mat &mat) {
    if (!view_) return;
    if (mat.empty()) return;

    cv::Mat rgb;
    cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
    QImage img(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888);
    view_->setPreviewImage(img.copy());  // copy 避免 mat 生命周期影响 UI 显示
}

void MainWindowController::onSettings_() {
    if (!view_) return;

    ConfigDialog dlg(config_, view_);
    if (dlg.exec() != QDialog::Accepted) return;

    config_ = dlg.config();
    saveConfig_();
    view_->setStatusText(QStringLiteral("配置已保存 ✅"));

    // 如果当前已经选择视频，则提示用户重新加载（这里直接自动重置）
    if (!current_video_path_.isEmpty()) {
        resetEngine_(current_video_path_);
    }
}
