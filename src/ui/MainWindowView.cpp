#include "ui/MainWindowView.h"

#include <QAbstractSpinBox>
#include <QColorDialog>
#include <QEvent>
#include <QFileDialog>
#include <QImage>
#include <QLatin1Char>
#include <QPixmap>
#include <QRegularExpression>
#include <QStringList>
#include <algorithm>

#include <opencv2/core.hpp>

#include "ui_MainWindow.h"

MainWindowView::MainWindowView(QWidget *parent)
    : QMainWindow(parent), ui_(new Ui::MainWindow) {
    ui_->setupUi(this);

    // 选择视频按钮
    connect(ui_->videoBrowseButton, &QPushButton::clicked, this, &MainWindowView::openVideoRequested);

    // 开始/暂停按钮（主按钮）
    connect(ui_->startButton, &QPushButton::clicked, this, &MainWindowView::startRequested);
    connect(ui_->stopButton, &QPushButton::clicked, this, &MainWindowView::stopRequested);

    // 配置路径浏览按钮
    connect(ui_->detectorModelBrowseButton, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(this, "选择检测模型", ui_->detectorModelPathEdit->text(), "ONNX (*.onnx)");
        if (!path.isEmpty()) ui_->detectorModelPathEdit->setText(path);
    });
    connect(ui_->featureModelBrowseButton, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(this, "选择特征模型", ui_->featureModelPathEdit->text(), "ONNX (*.onnx)");
        if (!path.isEmpty()) ui_->featureModelPathEdit->setText(path);
    });
    connect(ui_->statsCsvBrowseButton, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getSaveFileName(this, "选择统计输出 CSV", ui_->statsCsvPathEdit->text(), "CSV (*.csv)");
        if (!path.isEmpty()) ui_->statsCsvPathEdit->setText(path);
    });

    // 源类型切换时联动 UI（视频/摄像头）
    connect(ui_->sourceCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        ui_->sourceStack->setCurrentIndex(index);
        emit sourceTypeChanged();
    });

    // 确保初始页与下拉选择一致
    ui_->sourceStack->setCurrentIndex(ui_->sourceCombo->currentIndex());

    connect(ui_->cameraIndexSpin, qOverload<int>(&QSpinBox::valueChanged), this, &MainWindowView::cameraIndexChanged);
    connect(ui_->sampleFpsSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &MainWindowView::sampleFpsChanged);
    connect(ui_->trackingCheck, &QCheckBox::toggled, this, &MainWindowView::trackingToggled);

    // ROI 颜色选择器
    connect(ui_->vizRoiColorButton, &QPushButton::clicked, this, [this]() {
        const QColor chosen = QColorDialog::getColor(roi_color_qt_.isValid() ? roi_color_qt_ : QColor(255, 215, 0),
                                                    this, "选择 ROI 颜色");
        if (chosen.isValid()) {
            roi_color_qt_ = chosen;
            applyRoiColorStyle_();
        }
    });

    // 禁用数值框滚轮修改（避免误触）
    const auto spinBoxes = findChildren<QAbstractSpinBox *>();
    for (auto *spin : spinBoxes) {
        spin->installEventFilter(this);
    }

    setStatusText(QStringLiteral("就绪"));
}

MainWindowView::~MainWindowView() {
    delete ui_;
    ui_ = nullptr;
}

void MainWindowView::setPreviewImage(const QImage &img) {
    if (!ui_) return;
    if (img.isNull()) return;

    ui_->previewLabel->setPixmap(
        QPixmap::fromImage(img).scaled(
            ui_->previewLabel->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        )
    );
}

void MainWindowView::setStatusText(const QString &text) {
    statusBar()->showMessage(text);
}

void MainWindowView::setVideoName(const QString &name) {
    if (!ui_) return;
    ui_->videoNameLabel->setText(name);
}

void MainWindowView::setVideoPath(const QString &path) {
    if (!ui_) return;
    ui_->videoPathEdit->setText(path);
}

void MainWindowView::setFrameInfo(const QString &text) {
    if (!ui_) return;
    ui_->frameInfoLabel->setText(text);
}

void MainWindowView::setProgress(int value, int maximum) {
    if (!ui_) return;
    if (maximum < 0) {
        ui_->progressBar->setVisible(false);
        return;
    }
    ui_->progressBar->setVisible(true);
    if (maximum == 0) {
        // 未知总帧数时显示“忙碌”进度条
        ui_->progressBar->setRange(0, 0);
        return;
    }
    ui_->progressBar->setRange(0, maximum);
    ui_->progressBar->setValue(std::min(value, maximum));
}

void MainWindowView::setStartButtonText(const QString &text) {
    if (!ui_) return;
    ui_->startButton->setText(text);
}

void MainWindowView::setInputControlsEnabled(bool enabled) {
    if (!ui_) return;
    ui_->sourceGroup->setEnabled(enabled);
    ui_->optionsGroup->setEnabled(enabled);
    ui_->settingsTabs->setEnabled(enabled);
}

void MainWindowView::setStartButtonEnabled(bool enabled) {
    if (!ui_) return;
    ui_->startButton->setEnabled(enabled);
}

void MainWindowView::setStopButtonEnabled(bool enabled) {
    if (!ui_) return;
    ui_->stopButton->setEnabled(enabled);
}

bool MainWindowView::isVideoSource() const {
    if (!ui_) return true;
    return ui_->sourceCombo->currentIndex() == 0;
}

QString MainWindowView::videoPath() const {
    if (!ui_) return QString();
    return ui_->videoPathEdit->text();
}

int MainWindowView::cameraIndex() const {
    if (!ui_) return 0;
    return ui_->cameraIndexSpin->value();
}

double MainWindowView::sampleFps() const {
    if (!ui_) return 0.0;
    return ui_->sampleFpsSpin->value();
}

bool MainWindowView::trackingEnabled() const {
    if (!ui_) return true;
    return ui_->trackingCheck->isChecked();
}

void MainWindowView::applyRoiColorStyle_() {
    if (!ui_) return;
    if (!roi_color_qt_.isValid()) {
        roi_color_qt_ = QColor(255, 215, 0);
    }
    const int r = roi_color_qt_.red();
    const int g = roi_color_qt_.green();
    const int b = roi_color_qt_.blue();
    const int luminance = (r * 299 + g * 587 + b * 114) / 1000;
    const QString textColor = (luminance < 140) ? "#ffffff" : "#1f2937";
    ui_->vizRoiColorButton->setText(QString("#%1%2%3")
                                        .arg(r, 2, 16, QLatin1Char('0'))
                                        .arg(g, 2, 16, QLatin1Char('0'))
                                        .arg(b, 2, 16, QLatin1Char('0'))
                                        .toUpper());
    ui_->vizRoiColorButton->setStyleSheet(
        QString("QPushButton { background-color: rgb(%1,%2,%3); color: %4; border: 1px solid #cbd5e1; }")
            .arg(r).arg(g).arg(b).arg(textColor));
}

bool MainWindowView::eventFilter(QObject *obj, QEvent *event) {
    if (event && event->type() == QEvent::Wheel) {
        if (qobject_cast<QAbstractSpinBox *>(obj)) {
            event->ignore();
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindowView::loadConfig(const AppConfig &cfg) {
    if (!ui_) return;
    cached_config_ = cfg;

    // 检测器参数
    ui_->detectorInputWidthSpin->setValue(cfg.engine.detector.input_width);
    ui_->detectorInputHeightSpin->setValue(cfg.engine.detector.input_height);
    ui_->detectorScoreThrSpin->setValue(cfg.engine.detector.score_threshold);
    ui_->detectorNmsThrSpin->setValue(cfg.engine.detector.nms_threshold);
    ui_->detectorFocusIdsEdit->setText(joinIntList_(cfg.engine.detector.focus_class_ids));
    ui_->detectorFilterEdgeCheck->setChecked(cfg.engine.detector.filter_edge_boxes);
    ui_->detectorModelPathEdit->setText(QString::fromStdString(cfg.engine.detector.ort_env_config.model_path));
    ui_->detectorUseGpuCheck->setChecked(cfg.engine.detector.ort_env_config.using_gpu);
    ui_->detectorDeviceIdSpin->setValue(cfg.engine.detector.ort_env_config.device_id);
    ui_->detectorGpuMemSpin->setValue(static_cast<double>(cfg.engine.detector.ort_env_config.gpu_mem_limit) / (1024.0 * 1024.0));

    // 特征提取器参数
    ui_->featureInputWidthSpin->setValue(cfg.engine.extractor.input_width);
    ui_->featureInputHeightSpin->setValue(cfg.engine.extractor.input_height);
    ui_->featureModelPathEdit->setText(QString::fromStdString(cfg.engine.extractor.ort_env_config.model_path));
    ui_->featureUseGpuCheck->setChecked(cfg.engine.extractor.ort_env_config.using_gpu);
    ui_->featureDeviceIdSpin->setValue(cfg.engine.extractor.ort_env_config.device_id);
    ui_->featureGpuMemSpin->setValue(static_cast<double>(cfg.engine.extractor.ort_env_config.gpu_mem_limit) / (1024.0 * 1024.0));

    // 跟踪器参数
    ui_->trackerIouWeightSpin->setValue(cfg.engine.tracker_mgr.matcher_cfg.iou_weight);
    ui_->trackerFeatWeightSpin->setValue(cfg.engine.tracker_mgr.matcher_cfg.feature_weight);
    ui_->trackerMatchThrSpin->setValue(cfg.engine.tracker_mgr.matcher_cfg.threshold);
    ui_->trackerMaxLifeSpin->setValue(cfg.engine.tracker_mgr.tracker_cfg.max_life);
    ui_->trackerMomentumSpin->setValue(cfg.engine.tracker_mgr.tracker_cfg.feature_momentum);
    ui_->trackerHealthyPercentSpin->setValue(cfg.engine.tracker_mgr.tracker_cfg.healthy_percent);
    ui_->trackerKfPosNoiseSpin->setValue(cfg.engine.tracker_mgr.tracker_cfg.kf_pos_noise);
    ui_->trackerKfSizeNoiseSpin->setValue(cfg.engine.tracker_mgr.tracker_cfg.kf_size_noise);

    // ROI 参数
    ui_->roiEnableCheck->setChecked(cfg.engine.roi.enabled);
    ui_->roiXSpin->setValue(cfg.engine.roi.x);
    ui_->roiYSpin->setValue(cfg.engine.roi.y);
    ui_->roiWSpin->setValue(cfg.engine.roi.w);
    ui_->roiHSpin->setValue(cfg.engine.roi.h);

    // 记录器参数
    ui_->statsCsvPathEdit->setText(QString::fromStdString(cfg.recorder.stats_csv_path));
    ui_->statsExtraCheck->setChecked(cfg.recorder.enable_extra_statistics);

    // 可视化参数
    ui_->vizBoxThicknessSpin->setValue(cfg.visualizer.box_thickness);
    ui_->vizTextThicknessSpin->setValue(cfg.visualizer.text_thickness);
    ui_->vizFontScaleSpin->setValue(cfg.visualizer.font_scale);
    ui_->vizTextPaddingSpin->setValue(cfg.visualizer.text_padding);
    ui_->vizShowScoreCheck->setChecked(cfg.visualizer.show_score);
    ui_->vizShowClassIdCheck->setChecked(cfg.visualizer.show_class_id);
    ui_->vizRoiAlphaSpin->setValue(cfg.visualizer.roi_fill_alpha);
    roi_color_qt_ = QColor(static_cast<int>(cfg.visualizer.roi_color[2]),
                           static_cast<int>(cfg.visualizer.roi_color[1]),
                           static_cast<int>(cfg.visualizer.roi_color[0]));
    applyRoiColorStyle_();
}

AppConfig MainWindowView::collectConfig() const {
    AppConfig cfg = cached_config_;
    if (!ui_) return cfg;

    // 检测器参数
    cfg.engine.detector.input_width = ui_->detectorInputWidthSpin->value();
    cfg.engine.detector.input_height = ui_->detectorInputHeightSpin->value();
    cfg.engine.detector.score_threshold = static_cast<float>(ui_->detectorScoreThrSpin->value());
    cfg.engine.detector.nms_threshold = static_cast<float>(ui_->detectorNmsThrSpin->value());
    cfg.engine.detector.focus_class_ids = parseIntList_(ui_->detectorFocusIdsEdit->text());
    cfg.engine.detector.filter_edge_boxes = ui_->detectorFilterEdgeCheck->isChecked();
    cfg.engine.detector.ort_env_config.model_path = ui_->detectorModelPathEdit->text().toStdString();
    cfg.engine.detector.ort_env_config.using_gpu = ui_->detectorUseGpuCheck->isChecked();
    cfg.engine.detector.ort_env_config.device_id = ui_->detectorDeviceIdSpin->value();
    cfg.engine.detector.ort_env_config.gpu_mem_limit = static_cast<size_t>(ui_->detectorGpuMemSpin->value() * 1024.0 * 1024.0);

    // 特征提取器参数
    cfg.engine.extractor.input_width = ui_->featureInputWidthSpin->value();
    cfg.engine.extractor.input_height = ui_->featureInputHeightSpin->value();
    cfg.engine.extractor.ort_env_config.model_path = ui_->featureModelPathEdit->text().toStdString();
    cfg.engine.extractor.ort_env_config.using_gpu = ui_->featureUseGpuCheck->isChecked();
    cfg.engine.extractor.ort_env_config.device_id = ui_->featureDeviceIdSpin->value();
    cfg.engine.extractor.ort_env_config.gpu_mem_limit = static_cast<size_t>(ui_->featureGpuMemSpin->value() * 1024.0 * 1024.0);

    // 跟踪器参数
    cfg.engine.tracker_mgr.matcher_cfg.iou_weight = ui_->trackerIouWeightSpin->value();
    cfg.engine.tracker_mgr.matcher_cfg.feature_weight = ui_->trackerFeatWeightSpin->value();
    cfg.engine.tracker_mgr.matcher_cfg.threshold = ui_->trackerMatchThrSpin->value();
    cfg.engine.tracker_mgr.tracker_cfg.max_life = ui_->trackerMaxLifeSpin->value();
    cfg.engine.tracker_mgr.tracker_cfg.feature_momentum = ui_->trackerMomentumSpin->value();
    cfg.engine.tracker_mgr.tracker_cfg.healthy_percent = static_cast<float>(ui_->trackerHealthyPercentSpin->value());
    cfg.engine.tracker_mgr.tracker_cfg.kf_pos_noise = static_cast<float>(ui_->trackerKfPosNoiseSpin->value());
    cfg.engine.tracker_mgr.tracker_cfg.kf_size_noise = static_cast<float>(ui_->trackerKfSizeNoiseSpin->value());

    // ROI 参数
    cfg.engine.roi.enabled = ui_->roiEnableCheck->isChecked();
    cfg.engine.roi.x = static_cast<float>(ui_->roiXSpin->value());
    cfg.engine.roi.y = static_cast<float>(ui_->roiYSpin->value());
    cfg.engine.roi.w = static_cast<float>(ui_->roiWSpin->value());
    cfg.engine.roi.h = static_cast<float>(ui_->roiHSpin->value());

    // 记录器参数
    cfg.recorder.stats_csv_path = ui_->statsCsvPathEdit->text().toStdString();
    cfg.recorder.enable_extra_statistics = ui_->statsExtraCheck->isChecked();

    // 可视化参数
    cfg.visualizer.box_thickness = ui_->vizBoxThicknessSpin->value();
    cfg.visualizer.text_thickness = ui_->vizTextThicknessSpin->value();
    cfg.visualizer.font_scale = ui_->vizFontScaleSpin->value();
    cfg.visualizer.text_padding = ui_->vizTextPaddingSpin->value();
    cfg.visualizer.show_score = ui_->vizShowScoreCheck->isChecked();
    cfg.visualizer.show_class_id = ui_->vizShowClassIdCheck->isChecked();
    cfg.visualizer.roi_fill_alpha = static_cast<float>(ui_->vizRoiAlphaSpin->value());
    const QColor color = roi_color_qt_.isValid() ? roi_color_qt_ : QColor(255, 215, 0);
    cfg.visualizer.roi_color = cv::Scalar(
        color.blue(),
        color.green(),
        color.red()
    );

    return cfg;
}

QString MainWindowView::joinIntList_(const std::vector<int> &ids) {
    QStringList parts;
    parts.reserve(static_cast<int>(ids.size()));
    for (int id : ids) parts << QString::number(id);
    return parts.join(", ");
}

std::vector<int> MainWindowView::parseIntList_(const QString &text) {
    const QStringList tokens = text.split(QRegularExpression("[,\\s]+"), Qt::SkipEmptyParts);
    std::vector<int> ids;
    ids.reserve(static_cast<size_t>(tokens.size()));
    for (const auto &t : tokens) {
        bool ok = false;
        const int v = t.toInt(&ok);
        if (ok) ids.push_back(v);
    }
    return ids;
}
