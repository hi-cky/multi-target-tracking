#pragma once

#include <QMainWindow>
#include <vector>

#include "config/AppConfig.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

// MainWindowView 只负责界面布局/样式与基础信号，不承载业务逻辑
class MainWindowView final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindowView(QWidget *parent = nullptr);
    ~MainWindowView() override;

    // 更新预览画面
    void setPreviewImage(const QImage &img);
    // 更新状态栏文本
    void setStatusText(const QString &text);
    // 更新顶部视频名称
    void setVideoName(const QString &name);
    // 更新视频路径显示
    void setVideoPath(const QString &path);
    // 更新帧号显示
    void setFrameInfo(const QString &text);
    // 更新进度条（max<=0 表示隐藏）
    void setProgress(int value, int maximum);
    // 更新开始按钮文字
    void setStartButtonText(const QString &text);
    // 输入控制区启用/禁用
    void setInputControlsEnabled(bool enabled);
    // 开始按钮启用/禁用
    void setStartButtonEnabled(bool enabled);
    // 结束按钮启用/禁用
    void setStopButtonEnabled(bool enabled);

    // 读取当前 UI 选择
    bool isVideoSource() const;
    QString videoPath() const;
    int cameraIndex() const;
    double sampleFps() const;
    bool trackingEnabled() const;

    // 将配置加载到左侧设置面板
    void loadConfig(const AppConfig &cfg);
    // 从左侧设置面板读取配置
    AppConfig collectConfig() const;

signals:
    void openVideoRequested();
    void startRequested();
    void stopRequested();
    void sourceTypeChanged();
    void cameraIndexChanged(int index);
    void sampleFpsChanged(double fps);
    void trackingToggled(bool enabled);

private:
    bool eventFilter(QObject *obj, QEvent *event) override;

    static QString joinIntList_(const std::vector<int> &ids);
    static std::vector<int> parseIntList_(const QString &text);

    Ui::MainWindow *ui_ = nullptr;
    AppConfig cached_config_{};
};
