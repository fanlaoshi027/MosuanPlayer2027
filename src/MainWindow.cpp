#include "MainWindow.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSlider>
#include <QSplitter>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

namespace {
QLabel *sectionTitle(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("sectionTitle"));
    return label;
}

QToolButton *iconButton(QStyle::StandardPixmap icon, const QString &tooltip, QWidget *parent)
{
    auto *button = new QToolButton(parent);
    button->setIcon(parent->style()->standardIcon(icon));
    button->setToolTip(tooltip);
    button->setAutoRaise(true);
    return button;
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("墨算播放器"));
    resize(1280, 760);
    setMinimumSize(960, 600);

    buildUi();
}

void MainWindow::buildUi()
{
    auto *central = new QWidget(this);
    auto *root = new QHBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *splitter = new QSplitter(Qt::Horizontal, central);
    splitter->addWidget(createSidebar());
    splitter->addWidget(createVideoArea());
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({280, 1000});

    root->addWidget(splitter);
    setCentralWidget(central);

    setStyleSheet(QStringLiteral(
        "QMainWindow, QWidget { background: #17191d; color: #e9edf2; }"
        "QSplitter::handle { background: #2a2e35; width: 1px; }"
        "#sidebar { background: #202329; }"
        "#brand { font-size: 20px; font-weight: 700; padding: 4px 0 10px 0; }"
        "#sectionTitle { color: #aeb6c2; font-size: 12px; font-weight: 600; padding: 8px 4px 4px 4px; }"
        "QListWidget { border: none; background: transparent; outline: none; padding: 0; }"
        "QListWidget::item { padding: 8px 10px; border-radius: 5px; }"
        "QListWidget::item:selected { background: #343a44; }"
        "QListWidget::item:hover { background: #2b3038; }"
        "#videoFrame { background: #0b0c0f; border: 1px solid #2d323a; }"
        "#videoPlaceholder { color: #68717d; font-size: 18px; }"
        "#controlBar { background: #202329; border-top: 1px solid #2d323a; }"
        "#currentFile { color: #cbd2dc; }"
        "QSlider::groove:horizontal { height: 4px; background: #3a4049; border-radius: 2px; }"
        "QSlider::handle:horizontal { width: 12px; margin: -4px 0; border-radius: 6px; background: #d7dce3; }"
        "QToolButton { color: #d7dce3; padding: 5px; }"
        "QToolButton:hover { background: #343a44; border-radius: 4px; }"
        "QPushButton { background: #2b3038; border: 1px solid #3a414c; border-radius: 5px; padding: 6px 10px; }"
        "QPushButton:hover { background: #343a44; }"
    ));
}

QWidget *MainWindow::createSidebar()
{
    auto *sidebar = new QWidget(this);
    sidebar->setObjectName(QStringLiteral("sidebar"));
    sidebar->setMinimumWidth(240);
    sidebar->setMaximumWidth(360);

    auto *layout = new QVBoxLayout(sidebar);
    layout->setContentsMargins(14, 14, 10, 12);
    layout->setSpacing(4);

    auto *brand = new QLabel(QStringLiteral("墨算播放器"), sidebar);
    brand->setObjectName(QStringLiteral("brand"));
    layout->addWidget(brand);

    auto *openFolder = new QPushButton(QStringLiteral("＋ 添加文件夹播放列表"), sidebar);
    layout->addWidget(openFolder);

    layout->addWidget(sectionTitle(QStringLiteral("播放列表 1"), sidebar));
    m_playlist1 = new QListWidget(sidebar);
    m_playlist1->addItem(QStringLiteral("尚未指定文件夹"));
    layout->addWidget(m_playlist1, 1);

    layout->addWidget(sectionTitle(QStringLiteral("播放列表 2"), sidebar));
    m_playlist2 = new QListWidget(sidebar);
    m_playlist2->addItem(QStringLiteral("尚未指定文件夹"));
    layout->addWidget(m_playlist2, 1);

    layout->addWidget(sectionTitle(QStringLiteral("文件夹收藏夹"), sidebar));
    m_favorites = new QListWidget(sidebar);
    m_favorites->addItem(QStringLiteral("暂无收藏文件夹"));
    layout->addWidget(m_favorites, 1);

    auto *hint = new QLabel(
        QStringLiteral("SMB：打开列表时更新一次\n播放过程中不后台扫描"),
        sidebar);
    hint->setStyleSheet(QStringLiteral("color:#7f8996; font-size:11px; padding-top:6px;"));
    layout->addWidget(hint);

    return sidebar;
}

QWidget *MainWindow::createVideoArea()
{
    auto *container = new QWidget(this);
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *videoFrame = new QFrame(container);
    videoFrame->setObjectName(QStringLiteral("videoFrame"));

    auto *videoLayout = new QVBoxLayout(videoFrame);
    videoLayout->setContentsMargins(0, 0, 0, 0);

    m_videoPlaceholder = new QLabel(QStringLiteral("视频播放区域"), videoFrame);
    m_videoPlaceholder->setObjectName(QStringLiteral("videoPlaceholder"));
    m_videoPlaceholder->setAlignment(Qt::AlignCenter);
    videoLayout->addWidget(m_videoPlaceholder);

    layout->addWidget(videoFrame, 1);
    layout->addWidget(createControlBar());

    return container;
}

QWidget *MainWindow::createControlBar()
{
    auto *bar = new QWidget(this);
    bar->setObjectName(QStringLiteral("controlBar"));
    bar->setMinimumHeight(92);

    auto *layout = new QVBoxLayout(bar);
    layout->setContentsMargins(14, 8, 14, 8);
    layout->setSpacing(5);

    auto *top = new QHBoxLayout;
    m_currentFile = new QLabel(QStringLiteral("未播放视频"), bar);
    m_currentFile->setObjectName(QStringLiteral("currentFile"));
    top->addWidget(m_currentFile);
    top->addStretch();

    auto *speed = new QPushButton(QStringLiteral("1.0×"), bar);
    speed->setToolTip(QStringLiteral("播放速度"));
    top->addWidget(speed);

    auto *invert = new QPushButton(QStringLiteral("反色"), bar);
    invert->setToolTip(QStringLiteral("视频反色（后续接入 GPU Shader）"));
    top->addWidget(invert);

    auto *crop = new QPushButton(QStringLiteral("裁切"), bar);
    crop->setToolTip(QStringLiteral("画面裁切（后续接入）"));
    top->addWidget(crop);

    layout->addLayout(top);

    m_progress = new QSlider(Qt::Horizontal, bar);
    m_progress->setRange(0, 1000);
    m_progress->setValue(0);
    layout->addWidget(m_progress);

    auto *bottom = new QHBoxLayout;
    bottom->addWidget(iconButton(QStyle::SP_MediaSkipBackward, QStringLiteral("上一个"), bar));
    bottom->addWidget(iconButton(QStyle::SP_MediaPlay, QStringLiteral("播放/暂停"), bar));
    bottom->addWidget(iconButton(QStyle::SP_MediaSkipForward, QStringLiteral("下一个"), bar));
    bottom->addStretch();
    bottom->addWidget(new QLabel(QStringLiteral("音量"), bar));
    bottom->addWidget(iconButton(QStyle::SP_MediaVolume, QStringLiteral("音量"), bar));
    bottom->addWidget(iconButton(QStyle::SP_TitleBarMaxButton, QStringLiteral("全屏"), bar));

    layout->addLayout(bottom);
    return bar;
}
