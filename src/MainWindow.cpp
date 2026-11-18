#include "MainWindow.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>
#include <QSlider>
#include <QSplitter>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

namespace {
const QStringList kVideoExtensions = {
    QStringLiteral("mp4"), QStringLiteral("mkv"), QStringLiteral("mov"),
    QStringLiteral("avi"), QStringLiteral("wmv"), QStringLiteral("flv"),
    QStringLiteral("webm"), QStringLiteral("m4v"), QStringLiteral("ts"),
    QStringLiteral("m2ts")
};

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

bool isVideoFile(const QFileInfo &info)
{
    return kVideoExtensions.contains(info.suffix().toLower());
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
    splitter->setSizes({320, 960});

    root->addWidget(splitter);
    setCentralWidget(central);

    setStyleSheet(QStringLiteral(
        "QMainWindow, QWidget { background: #17191d; color: #e9edf2; }"
        "QSplitter::handle { background: #2a2e35; width: 1px; }"
        "#sidebar { background: #202329; }"
        "#brand { font-size: 20px; font-weight: 700; padding: 4px 0 8px 0; }"
        "#sectionTitle { color: #aeb6c2; font-size: 12px; font-weight: 600; padding: 8px 4px 3px 4px; }"
        "#pathLabel { color: #7f8996; font-size: 10px; padding: 0 4px 3px 4px; }"
        "QListWidget { border: none; background: transparent; outline: none; padding: 0; }"
        "QListWidget::item { padding: 7px 8px; border-radius: 5px; }"
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
        "QPushButton { background: #2b3038; border: 1px solid #3a414c; border-radius: 5px; padding: 5px 8px; }"
        "QPushButton:hover { background: #343a44; }"
    ));
}

QWidget *MainWindow::createSidebar()
{
    auto *sidebar = new QWidget(this);
    sidebar->setObjectName(QStringLiteral("sidebar"));
    sidebar->setMinimumWidth(280);
    sidebar->setMaximumWidth(420);

    auto *layout = new QVBoxLayout(sidebar);
    layout->setContentsMargins(12, 12, 10, 12);
    layout->setSpacing(3);

    auto *brand = new QLabel(QStringLiteral("墨算播放器"), sidebar);
    brand->setObjectName(QStringLiteral("brand"));
    layout->addWidget(brand);

    auto *openFolder = new QPushButton(QStringLiteral("＋ 添加文件夹播放列表"), sidebar);
    layout->addWidget(openFolder);

    auto makePlaylistPanel = [this, layout](const QString &title, int index, QListWidget **outList) {
        layout->addWidget(sectionTitle(title, this));

        auto *path = new QLabel(QStringLiteral("尚未指定文件夹"), this);
        path->setObjectName(QStringLiteral("pathLabel"));
        path->setWordWrap(true);
        layout->addWidget(path);

        auto *buttons = new QHBoxLayout;
        auto *choose = new QPushButton(QStringLiteral("选择"), this);
        auto *refresh = new QPushButton(QStringLiteral("打开/刷新"), this);
        auto *favorite = new QPushButton(QStringLiteral("收藏"), this);
        buttons->addWidget(choose);
        buttons->addWidget(refresh);
        buttons->addWidget(favorite);
        layout->addLayout(buttons);

        auto *list = new QListWidget(this);
        list->setMinimumHeight(95);
        layout->addWidget(list, 1);
        *outList = list;

        auto settingsKey = QStringLiteral("playlist%1/path").arg(index);
        QSettings settings;
        const QString savedPath = settings.value(settingsKey).toString();
        if (!savedPath.isEmpty()) {
            path->setText(savedPath);
        }

        auto scan = [this, index, list, path]() {
            QSettings settings;
            const QString folder = settings.value(QStringLiteral("playlist%1/path").arg(index)).toString();
            if (folder.isEmpty()) {
                list->clear();
                list->addItem(QStringLiteral("请先选择文件夹"));
                return;
            }

            QDir dir(folder);
            if (!dir.exists()) {
                list->clear();
                list->addItem(QStringLiteral("文件夹不可用"));
                path->setText(folder + QStringLiteral("  [离线]"));
                return;
            }

            const QFileInfoList files = dir.entryInfoList(
                QDir::Files | QDir::Readable,
                QDir::Name | QDir::IgnoreCase);

            list->clear();
            for (const QFileInfo &info : files) {
                if (isVideoFile(info)) {
                    auto *item = new QListWidgetItem(info.fileName(), list);
                    item->setToolTip(info.absoluteFilePath());
                    item->setData(Qt::UserRole, info.absoluteFilePath());
                }
            }

            if (list->count() == 0) {
                list->addItem(QStringLiteral("该文件夹没有支持的视频"));
            }
            path->setText(folder);
        };

        connect(choose, &QPushButton::clicked, this, [this, index, path, scan]() {
            QSettings settings;
            const QString current = settings.value(QStringLiteral("playlist%1/path").arg(index)).toString();
            const QString folder = QFileDialog::getExistingDirectory(
                this, QStringLiteral("选择播放列表 %1 文件夹").arg(index),
                current);
            if (folder.isEmpty()) {
                return;
            }
            settings.setValue(QStringLiteral("playlist%1/path").arg(index), folder);
            path->setText(folder);
            scan();
        });

        connect(refresh, &QPushButton::clicked, this, scan);
        connect(favorite, &QPushButton::clicked, this, [this, index]() {
            QSettings settings;
            const QString folder = settings.value(QStringLiteral("playlist%1/path").arg(index)).toString();
            if (folder.isEmpty()) {
                return;
            }
            QStringList favorites = settings.value(QStringLiteral("favorites")).toStringList();
            if (!favorites.contains(folder)) {
                favorites.append(folder);
                settings.setValue(QStringLiteral("favorites"), favorites);
                m_favorites->clear();
                for (const QString &item : favorites) {
                    m_favorites->addItem(item);
                }
            }
        });

        connect(list, &QListWidget::itemDoubleClicked, this,
                [this](QListWidgetItem *item) {
                    const QString file = item->data(Qt::UserRole).toString();
                    if (!file.isEmpty()) {
                        m_currentFile->setText(file);
                        m_videoPlaceholder->setText(QStringLiteral("已选择视频\n%1\n\n下一阶段接入 FFmpeg 播放").arg(file));
                    }
                });
    };

    makePlaylistPanel(QStringLiteral("播放列表 1"), 1, &m_playlist1);
    makePlaylistPanel(QStringLiteral("播放列表 2"), 2, &m_playlist2);

    layout->addWidget(sectionTitle(QStringLiteral("文件夹收藏夹"), sidebar));
    m_favorites = new QListWidget(sidebar);
    m_favorites->setMaximumHeight(125);
    layout->addWidget(m_favorites);

    QSettings settings;
    const QStringList favorites = settings.value(QStringLiteral("favorites")).toStringList();
    for (const QString &folder : favorites) {
        m_favorites->addItem(folder);
    }
    if (m_favorites->count() == 0) {
        m_favorites->addItem(QStringLiteral("暂无收藏文件夹"));
    }

    connect(m_favorites, &QListWidget::itemDoubleClicked, this,
            [this](QListWidgetItem *item) {
                const QString folder = item->text();
                if (folder.isEmpty() || folder == QStringLiteral("暂无收藏文件夹")) {
                    return;
                }
                QSettings settings;
                settings.setValue(QStringLiteral("playlist1/path"), folder);
                m_playlist1->clear();
                m_playlist1->addItem(QStringLiteral("已收藏：%1").arg(folder));
                m_playlist1->setCurrentRow(0);
            });

    auto *hint = new QLabel(
        QStringLiteral("SMB：打开/刷新时扫描一次\n播放过程中不后台扫描"),
        sidebar);
    hint->setStyleSheet(QStringLiteral("color:#7f8996; font-size:11px; padding-top:5px;"));
    layout->addWidget(hint);

    connect(openFolder, &QPushButton::clicked, this, [this]() {
        QSettings settings;
        const QString folder = QFileDialog::getExistingDirectory(this, QStringLiteral("选择文件夹"));
        if (folder.isEmpty()) {
            return;
        }
        settings.setValue(QStringLiteral("playlist1/path"), folder);
        m_playlist1->clear();
        m_playlist1->addItem(QStringLiteral("已指定：%1").arg(folder));
    });

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
