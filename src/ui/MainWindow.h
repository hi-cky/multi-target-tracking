#pragma once

#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <memory>

#include "core/processor/TrackingEngine.h"
#include "core/visualizer/Visualizer.h"
#include "core/recorder/StatsRecorder.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onOpenVideo();
    void onStart();
    void onTick();

private:
    void resetEngine(const QString &path);
    void showMat(const cv::Mat &mat);

    QLabel *video_label_ = nullptr;
    QPushButton *open_btn_ = nullptr;
    QPushButton *start_btn_ = nullptr;
    QTimer timer_;

    std::unique_ptr<TrackingEngine> engine_;
    std::unique_ptr<ILabeledDataIterator> iterator_;
    Visualizer viz_;
    std::unique_ptr<StatsRecorder> stats_;
};
