#pragma once

#include <QDialog>

#include "config/AppConfig.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class ConfigDialog;
}
QT_END_NAMESPACE

// 中文注释：配置界面（只负责把 AppConfig 映射到表单，并在确认时输出新配置）
class ConfigDialog final : public QDialog {
    Q_OBJECT
public:
    explicit ConfigDialog(const AppConfig &initial, QWidget *parent = nullptr);
    ~ConfigDialog() override;

    AppConfig config() const;

private:
    void loadToUi_(const AppConfig &cfg);
    AppConfig readFromUi_() const;

    static QString joinIntList_(const std::vector<int> &ids);
    static std::vector<int> parseIntList_(const QString &text);

    Ui::ConfigDialog *ui_ = nullptr;
};
