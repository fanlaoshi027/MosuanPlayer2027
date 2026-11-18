#include "MainWindow.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("墨算播放器 · 技术验证版"));
    resize(1100, 680);

    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);

    auto *title = new QLabel(QStringLiteral("MosuanPlayer2027"), central);
    title->setStyleSheet(QStringLiteral("font-size: 22px; font-weight: 600;"));

    m_statusLabel = new QLabel(
        QStringLiteral("项目骨架已建立。下一阶段验证 Qt 6 + LGPL FFmpeg 播放管线。"),
        central);

    layout->addWidget(title);
    layout->addWidget(m_statusLabel);
    layout->addStretch();

    setCentralWidget(central);
}
