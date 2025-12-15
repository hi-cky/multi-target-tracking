#pragma once

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

// 中文注释：MainWindowView 只负责界面布局/样式与基础信号，不承载业务逻辑
class MainWindowView final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindowView(QWidget *parent = nullptr);
    ~MainWindowView() override;

    // 中文注释：更新预览画面
    void setPreviewImage(const QImage &img);
    // 中文注释：更新状态栏文本
    void setStatusText(const QString &text);
    // 中文注释：更新按钮可用状态
    void setControlsEnabled(bool enabled);

signals:
    void openVideoRequested();
    void startRequested();
    void settingsRequested();

private:
    Ui::MainWindow *ui_ = nullptr;
};

