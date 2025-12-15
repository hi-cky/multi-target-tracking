#include "ui/MainWindowView.h"

#include <QImage>
#include <QPixmap>

#include "ui_MainWindow.h"

MainWindowView::MainWindowView(QWidget *parent)
    : QMainWindow(parent), ui_(new Ui::MainWindow) {
    ui_->setupUi(this);

    connect(ui_->openButton, &QPushButton::clicked, this, &MainWindowView::openVideoRequested);
    connect(ui_->startButton, &QPushButton::clicked, this, &MainWindowView::startRequested);
    connect(ui_->settingsButton, &QPushButton::clicked, this, &MainWindowView::settingsRequested);

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

void MainWindowView::setControlsEnabled(bool enabled) {
    if (!ui_) return;
    ui_->openButton->setEnabled(enabled);
    ui_->startButton->setEnabled(enabled);
    ui_->settingsButton->setEnabled(enabled);
}

