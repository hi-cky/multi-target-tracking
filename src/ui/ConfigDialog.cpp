#include "ui/ConfigDialog.h"

#include <QFileDialog>
#include <QStringList>
#include <QRegularExpression>

#include "ui_ConfigDialog.h"

ConfigDialog::ConfigDialog(const AppConfig &initial, QWidget *parent)
    : QDialog(parent), ui_(new Ui::ConfigDialog) {
    ui_->setupUi(this);

    // 中文注释：浏览按钮（选择 detector/feature 模型路径）
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

    loadToUi_(initial);
}

ConfigDialog::~ConfigDialog() {
    delete ui_;
    ui_ = nullptr;
}

AppConfig ConfigDialog::config() const {
    return readFromUi_();
}

void ConfigDialog::loadToUi_(const AppConfig &cfg) {
    // Detector
    ui_->detectorInputWidthSpin->setValue(cfg.engine.detector.input_width);
    ui_->detectorInputHeightSpin->setValue(cfg.engine.detector.input_height);
    ui_->detectorScoreThrSpin->setValue(cfg.engine.detector.score_threshold);
    ui_->detectorNmsThrSpin->setValue(cfg.engine.detector.nms_threshold);
    ui_->detectorFocusIdsEdit->setText(joinIntList_(cfg.engine.detector.focus_class_ids));
    ui_->detectorModelPathEdit->setText(QString::fromStdString(cfg.engine.detector.ort_env_config.model_path));
    ui_->detectorUseGpuCheck->setChecked(cfg.engine.detector.ort_env_config.using_gpu);
    ui_->detectorDeviceIdSpin->setValue(cfg.engine.detector.ort_env_config.device_id);
    ui_->detectorGpuMemSpin->setValue(static_cast<double>(cfg.engine.detector.ort_env_config.gpu_mem_limit) / (1024.0 * 1024.0));

    // Feature
    ui_->featureInputWidthSpin->setValue(cfg.engine.extractor.input_width);
    ui_->featureInputHeightSpin->setValue(cfg.engine.extractor.input_height);
    ui_->featureModelPathEdit->setText(QString::fromStdString(cfg.engine.extractor.ort_env_config.model_path));
    ui_->featureUseGpuCheck->setChecked(cfg.engine.extractor.ort_env_config.using_gpu);
    ui_->featureDeviceIdSpin->setValue(cfg.engine.extractor.ort_env_config.device_id);
    ui_->featureGpuMemSpin->setValue(static_cast<double>(cfg.engine.extractor.ort_env_config.gpu_mem_limit) / (1024.0 * 1024.0));

    // Tracker
    ui_->trackerIouWeightSpin->setValue(cfg.engine.tracker_mgr.matcher_cfg.iou_weight);
    ui_->trackerFeatWeightSpin->setValue(cfg.engine.tracker_mgr.matcher_cfg.feature_weight);
    ui_->trackerMatchThrSpin->setValue(cfg.engine.tracker_mgr.matcher_cfg.threshold);
    ui_->trackerMaxLifeSpin->setValue(cfg.engine.tracker_mgr.tracker_cfg.max_life);
    ui_->trackerMomentumSpin->setValue(cfg.engine.tracker_mgr.tracker_cfg.feature_momentum);

    // Recorder
    ui_->statsCsvPathEdit->setText(QString::fromStdString(cfg.recorder.stats_csv_path));
    ui_->statsExtraCheck->setChecked(cfg.recorder.enable_extra_statistics);
}

AppConfig ConfigDialog::readFromUi_() const {
    AppConfig cfg;

    // Detector
    cfg.engine.detector.input_width = ui_->detectorInputWidthSpin->value();
    cfg.engine.detector.input_height = ui_->detectorInputHeightSpin->value();
    cfg.engine.detector.score_threshold = static_cast<float>(ui_->detectorScoreThrSpin->value());
    cfg.engine.detector.nms_threshold = static_cast<float>(ui_->detectorNmsThrSpin->value());
    cfg.engine.detector.focus_class_ids = parseIntList_(ui_->detectorFocusIdsEdit->text());
    cfg.engine.detector.ort_env_config.model_path = ui_->detectorModelPathEdit->text().toStdString();
    cfg.engine.detector.ort_env_config.using_gpu = ui_->detectorUseGpuCheck->isChecked();
    cfg.engine.detector.ort_env_config.device_id = ui_->detectorDeviceIdSpin->value();
    cfg.engine.detector.ort_env_config.gpu_mem_limit = static_cast<size_t>(ui_->detectorGpuMemSpin->value() * 1024.0 * 1024.0);

    // Feature
    cfg.engine.extractor.input_width = ui_->featureInputWidthSpin->value();
    cfg.engine.extractor.input_height = ui_->featureInputHeightSpin->value();
    cfg.engine.extractor.ort_env_config.model_path = ui_->featureModelPathEdit->text().toStdString();
    cfg.engine.extractor.ort_env_config.using_gpu = ui_->featureUseGpuCheck->isChecked();
    cfg.engine.extractor.ort_env_config.device_id = ui_->featureDeviceIdSpin->value();
    cfg.engine.extractor.ort_env_config.gpu_mem_limit = static_cast<size_t>(ui_->featureGpuMemSpin->value() * 1024.0 * 1024.0);

    // Tracker
    cfg.engine.tracker_mgr.matcher_cfg.iou_weight = ui_->trackerIouWeightSpin->value();
    cfg.engine.tracker_mgr.matcher_cfg.feature_weight = ui_->trackerFeatWeightSpin->value();
    cfg.engine.tracker_mgr.matcher_cfg.threshold = ui_->trackerMatchThrSpin->value();
    cfg.engine.tracker_mgr.tracker_cfg.max_life = ui_->trackerMaxLifeSpin->value();
    cfg.engine.tracker_mgr.tracker_cfg.feature_momentum = ui_->trackerMomentumSpin->value();

    // Recorder
    cfg.recorder.stats_csv_path = ui_->statsCsvPathEdit->text().toStdString();
    cfg.recorder.enable_extra_statistics = ui_->statsExtraCheck->isChecked();

    return cfg;
}

QString ConfigDialog::joinIntList_(const std::vector<int> &ids) {
    QStringList parts;
    parts.reserve(static_cast<int>(ids.size()));
    for (int id : ids) parts << QString::number(id);
    return parts.join(", ");
}

std::vector<int> ConfigDialog::parseIntList_(const QString &text) {
    // 中文注释：支持 "0,1, 2" / "0 1 2" 等格式
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
