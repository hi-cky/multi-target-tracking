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
    ui_->detectorInputWidthSpin->setValue(cfg.detector.input_width);
    ui_->detectorInputHeightSpin->setValue(cfg.detector.input_height);
    ui_->detectorScoreThrSpin->setValue(cfg.detector.score_threshold);
    ui_->detectorNmsThrSpin->setValue(cfg.detector.nms_threshold);
    ui_->detectorFocusIdsEdit->setText(joinIntList_(cfg.detector.focus_class_ids));
    ui_->detectorModelPathEdit->setText(QString::fromStdString(cfg.detector.ort_env_config.model_path));
    ui_->detectorUseGpuCheck->setChecked(cfg.detector.ort_env_config.using_gpu);
    ui_->detectorDeviceIdSpin->setValue(cfg.detector.ort_env_config.device_id);
    ui_->detectorGpuMemSpin->setValue(static_cast<double>(cfg.detector.ort_env_config.gpu_mem_limit) / (1024.0 * 1024.0));

    // Feature
    ui_->featureInputWidthSpin->setValue(cfg.feature.input_width);
    ui_->featureInputHeightSpin->setValue(cfg.feature.input_height);
    ui_->featureModelPathEdit->setText(QString::fromStdString(cfg.feature.ort_env_config.model_path));
    ui_->featureUseGpuCheck->setChecked(cfg.feature.ort_env_config.using_gpu);
    ui_->featureDeviceIdSpin->setValue(cfg.feature.ort_env_config.device_id);
    ui_->featureGpuMemSpin->setValue(static_cast<double>(cfg.feature.ort_env_config.gpu_mem_limit) / (1024.0 * 1024.0));

    // Tracker
    ui_->trackerIouWeightSpin->setValue(cfg.tracker_mgr.iou_weight);
    ui_->trackerFeatWeightSpin->setValue(cfg.tracker_mgr.feature_weight);
    ui_->trackerMatchThrSpin->setValue(cfg.tracker_mgr.match_threshold);
    ui_->trackerMaxLifeSpin->setValue(cfg.tracker_mgr.max_life);
    ui_->trackerMomentumSpin->setValue(cfg.tracker_mgr.feature_momentum);

    // Misc
    ui_->statsCsvPathEdit->setText(QString::fromStdString(cfg.stats_csv_path));
}

AppConfig ConfigDialog::readFromUi_() const {
    AppConfig cfg;

    // Detector
    cfg.detector.input_width = ui_->detectorInputWidthSpin->value();
    cfg.detector.input_height = ui_->detectorInputHeightSpin->value();
    cfg.detector.score_threshold = static_cast<float>(ui_->detectorScoreThrSpin->value());
    cfg.detector.nms_threshold = static_cast<float>(ui_->detectorNmsThrSpin->value());
    cfg.detector.focus_class_ids = parseIntList_(ui_->detectorFocusIdsEdit->text());
    cfg.detector.ort_env_config.model_path = ui_->detectorModelPathEdit->text().toStdString();
    cfg.detector.ort_env_config.using_gpu = ui_->detectorUseGpuCheck->isChecked();
    cfg.detector.ort_env_config.device_id = ui_->detectorDeviceIdSpin->value();
    cfg.detector.ort_env_config.gpu_mem_limit = static_cast<size_t>(ui_->detectorGpuMemSpin->value() * 1024.0 * 1024.0);

    // Feature
    cfg.feature.input_width = ui_->featureInputWidthSpin->value();
    cfg.feature.input_height = ui_->featureInputHeightSpin->value();
    cfg.feature.ort_env_config.model_path = ui_->featureModelPathEdit->text().toStdString();
    cfg.feature.ort_env_config.using_gpu = ui_->featureUseGpuCheck->isChecked();
    cfg.feature.ort_env_config.device_id = ui_->featureDeviceIdSpin->value();
    cfg.feature.ort_env_config.gpu_mem_limit = static_cast<size_t>(ui_->featureGpuMemSpin->value() * 1024.0 * 1024.0);

    // Tracker
    cfg.tracker_mgr.iou_weight = ui_->trackerIouWeightSpin->value();
    cfg.tracker_mgr.feature_weight = ui_->trackerFeatWeightSpin->value();
    cfg.tracker_mgr.match_threshold = ui_->trackerMatchThrSpin->value();
    cfg.tracker_mgr.max_life = ui_->trackerMaxLifeSpin->value();
    cfg.tracker_mgr.feature_momentum = ui_->trackerMomentumSpin->value();

    // Misc
    cfg.stats_csv_path = ui_->statsCsvPathEdit->text().toStdString();

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
