#include <QApplication>
#include <QFile>
#include "ui/MainWindowView.h"
#include "ui/MainWindowController.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("multi-target-tracking");
    // 中文注释：加载 QSS 样式（样式与逻辑分离）
    QFile styleFile(":/ui/style.qss");
    if (styleFile.open(QIODevice::ReadOnly)) {
        app.setStyleSheet(QString::fromUtf8(styleFile.readAll()));
    }

    MainWindowView w;
    MainWindowController controller(&w);
    w.resize(800, 600);
    w.show();
    return app.exec();
}
