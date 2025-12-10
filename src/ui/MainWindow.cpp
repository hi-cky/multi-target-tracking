#include "ui/MainWindow.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QCoreApplication>
#include <opencv2/imgproc.hpp>

#include "core/processor/model/detector/YoloDetector.h"
#include "core/processor/model/feature_extractor/IFeatureExtractor.h"
#include "core/capture/VideoFrameSource.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    QWidget *central = new QWidget(this);
    auto *layout = new QVBoxLayout;

    auto *btnLayout = new QHBoxLayout;
    open_btn_ = new QPushButton("选择视频", this);
    start_btn_ = new QPushButton("开始", this);
    btnLayout->addWidget(open_btn_);
    btnLayout->addWidget(start_btn_);

    video_label_ = new QLabel("预览区域", this);
    video_label_->setAlignment(Qt::AlignCenter);
    video_label_->setMinimumSize(640, 360);

    layout->addLayout(btnLayout);
    layout->addWidget(video_label_);
    central->setLayout(layout);
    setCentralWidget(central);

    connect(open_btn_, &QPushButton::clicked, this, &MainWindow::onOpenVideo);
    connect(start_btn_, &QPushButton::clicked, this, &MainWindow::onStart);
    connect(&timer_, &QTimer::timeout, this, &MainWindow::onTick);
    timer_.setInterval(30);
}

void MainWindow::onOpenVideo() {
    QString path = QFileDialog::getOpenFileName(this, "选择视频文件", QString(), "Video (*.mp4 *.avi *.mov)");
    if (path.isEmpty()) return;
    resetEngine(path);
}

void MainWindow::resetEngine(const QString &path) {
    try {
        // 统一使用应用目录计算路径，避免工作目录差异导致模型/输出找不到
        // const QString appDir = QCoreApplication::applicationDirPath();
        // 先暂时设置为绝对路径
        const QString modelDir = "/Users/corn/code/c++/multi-target-tracking/model/";
        const std::string yolo_path = (modelDir + "yolo12n.onnx").toStdString();
        const std::string osnet_path = (modelDir + "osnet_x1_0.onnx").toStdString();
        const std::string csv_path = "/Users/corn/code/c++/multi-target-tracking/docs/output.csv";

        // 创建检测器
        DetectorConfig dcfg;
        dcfg.model_path = yolo_path;
        auto detector = std::make_unique<YoloDetector>(dcfg);

        // 创建特征提取器
        FeatureExtractorConfig fcfg;
        fcfg.model_path = osnet_path;
        auto extractor = CreateFeatureExtractor(fcfg);

        TrackingEngine::Config ecfg;
        ecfg.detector = dcfg;
        ecfg.feature = fcfg;
        ecfg.tracker_mgr = TrackerManagerConfig{};

        engine_ = std::make_unique<TrackingEngine>(std::move(detector), std::move(extractor), ecfg);

        VideoFrameSource src(path.toStdString());
        iterator_ = engine_->run(src.createIterator());

        stats_ = std::make_unique<StatsRecorder>(csv_path);
        video_label_->setText("就绪，点击开始");
    } catch (const std::exception &e) {
        QMessageBox::critical(this, "错误", e.what());
    }
}

void MainWindow::onStart() {
    if (!iterator_) {
        QMessageBox::information(this, "提示", "请先选择视频文件");
        return;
    }
    timer_.start();
}

void MainWindow::onTick() {
    if (!iterator_ || !iterator_->hasNext()) {
        timer_.stop();
        if (stats_) stats_->finalize();
        return;
    }
    LabeledFrame lf;
    if (!iterator_->next(lf)) {
        timer_.stop();
        if (stats_) stats_->finalize();
        return;
    }
    if (stats_) stats_->consume(lf);
    const cv::Mat &raw = iterator_->lastFrame();
    cv::Mat vis = viz_.render(raw, lf);
    showMat(vis);
}

void MainWindow::showMat(const cv::Mat &mat) {
    if (mat.empty()) return;
    cv::Mat rgb;
    cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
    QImage img(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888);
    video_label_->setPixmap(QPixmap::fromImage(img).scaled(video_label_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}
