#include "MainWindow.h"
#include "MpvEngine.h"

#include <QCollator>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
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
#include <QMessageBox>

#include <algorithm>

namespace {
struct FolderScanResult {
    bool success = false;
    QStringList files;
    QString error;
};

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

void restoreSnapshot(QListWidget *list, const QStringList &snapshot)
{
    list->clear();
    for (const QString &file : snapshot) {
        auto *item = new QListWidgetItem(QFileInfo(file).fileName(), list);
        item->setToolTip(file);
        item->setData(Qt::UserRole, file);
    }
    if (list->count() == 0)
        list->addItem(QStringLiteral("尚无扫描记录，请手动刷新"));
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("墨算播放器"));
    resize(1280, 760);
    setMinimumSize(960, 600);
    buildUi();
    m_mpv = new MpvEngine(this);
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

    auto makePlaylistPanel = [this, layout, sidebar](const QString &title, int index,
                                                       QListWidget **outList) {
        layout->addWidget(sectionTitle(title, sidebar));

        auto *pathLabel = new QLabel(QStringLiteral("尚未指定文件夹"), sidebar);
        pathLabel->setObjectName(QStringLiteral("pathLabel"));
        pathLabel->setWordWrap(true);
        layout->addWidget(pathLabel);

        auto *buttons = new QHBoxLayout;
        auto *choose = new QPushButton(QStringLiteral("选择文件夹"), sidebar);
        auto *refresh = new QPushButton(QStringLiteral("手动刷新"), sidebar);
        auto *favorite = new QPushButton(QStringLiteral("收藏"), sidebar);
        buttons->addWidget(choose);
        buttons->addWidget(refresh);
        buttons->addWidget(favorite);
        layout->addLayout(buttons);

        auto *list = new QListWidget(sidebar);
        list->setMinimumHeight(95);
        layout->addWidget(list, 1);
        *outList = list;

        if (index == 1) {
            m_playlist1PathLabel = pathLabel;
            m_playlist1RefreshButton = refresh;
        } else {
            m_playlist2PathLabel = pathLabel;
            m_playlist2RefreshButton = refresh;
        }

        QSettings settings;
        const QString pathKey = QStringLiteral("playlist%1/path").arg(index);
        const QString snapshotKey = QStringLiteral("playlist%1/snapshot").arg(index);
        const QString savedPath = settings.value(pathKey).toString();
        if (!savedPath.isEmpty()) {
            pathLabel->setText(savedPath);
            restoreSnapshot(list, settings.value(snapshotKey).toStringList());
        } else {
            list->addItem(QStringLiteral("请先选择文件夹"));
        }

        // Explicit scan only. On startup this is called once for each configured playlist.
        // Merely selecting a playlist or opening its panel never triggers an SMB scan.
        auto scan = [this, index, list, pathLabel, refresh]() {
            QSettings currentSettings;
            const QString folder = currentSettings.value(
                QStringLiteral("playlist%1/path").arg(index)).toString();
            if (folder.isEmpty()) {
                pathLabel->setText(QStringLiteral("尚未指定文件夹"));
                return;
            }

            refresh->setEnabled(false);
            refresh->setText(QStringLiteral("扫描中…"));

            auto *watcher = new QFutureWatcher<FolderScanResult>(this);
            connect(watcher, &QFutureWatcher<FolderScanResult>::finished, this,
                [watcher, index, list, pathLabel, refresh, folder]() {
                    const FolderScanResult result = watcher->result();
                    watcher->deleteLater();
                    refresh->setEnabled(true);
                    refresh->setText(QStringLiteral("手动刷新"));

                    if (!result.success) {
                        pathLabel->setText(folder + QStringLiteral("  [不可用；保留上次列表]"));
                        return;
                    }

                    QSettings saved;
                    saved.setValue(QStringLiteral("playlist%1/snapshot").arg(index), result.files);
                    restoreSnapshot(list, result.files);
                    pathLabel->setText(folder + QStringLiteral("  [%1 个视频]").arg(result.files.size()));
                });

            watcher->setFuture(QtConcurrent::run([folder]() {
                FolderScanResult result;
                QDir dir(folder);
                if (!dir.exists()) {
                    result.error = QStringLiteral("目录不存在或网络不可用");
                    return result;
                }

                QFileInfoList entries = dir.entryInfoList(
                    QDir::Files | QDir::Readable | QDir::NoDotAndDotDot);
                QCollator collator;
                collator.setNumericMode(true);
                collator.setCaseSensitivity(Qt::CaseInsensitive);
                std::sort(entries.begin(), entries.end(),
                    [&collator](const QFileInfo &a, const QFileInfo &b) {
                        return collator.compare(a.fileName(), b.fileName()) < 0;
                    });

                result.files.reserve(entries.size());
                for (const QFileInfo &info : entries) {
                    if (isVideoFile(info))
                        result.files.append(info.absoluteFilePath());
                }
                result.success = true;
                return result;
            }));
        };

        connect(choose, &QPushButton::clicked, this, [this, index, pathLabel, list]() {
            QSettings currentSettings;
            const QString current = currentSettings.value(
                QStringLiteral("playlist%1/path").arg(index)).toString();
            const QString folder = QFileDialog::getExistingDirectory(
                this, QStringLiteral("选择播放列表 %1 文件夹").arg(index), current);
            if (folder.isEmpty())
                return;

            currentSettings.setValue(QStringLiteral("playlist%1/path").arg(index), folder);
            pathLabel->setText(folder + QStringLiteral("  [已指定；手动刷新或下次启动时扫描]"));
            restoreSnapshot(list, currentSettings.value(
                QStringLiteral("playlist%1/snapshot").arg(index)).toStringList());
        });

        connect(refresh, &QPushButton::clicked, this, scan);

        connect(favorite, &QPushButton::clicked, this, [this, index]() {
            QSettings currentSettings;
            const QString folder = currentSettings.value(
                QStringLiteral("playlist%1/path").arg(index)).toString();
            if (folder.isEmpty())
                return;

            QStringList favorites = currentSettings.value(
                QStringLiteral("favorites")).toStringList();
            if (!favorites.contains(folder)) {
                favorites.append(folder);
                currentSettings.setValue(QStringLiteral("favorites"), favorites);
                m_favorites->clear();
                for (const QString &entry : favorites)
                    m_favorites->addItem(entry);
            }
        });

        connect(list, &QListWidget::itemDoubleClicked, this,
            [this](QListWidgetItem *item) {
                const QString file = item->data(Qt::UserRole).toString();
                if (file.isEmpty())
                    return;
                playFile(file);
            });

        // Refresh saved folders once when the application starts, never on panel activation.
        if (!savedPath.isEmpty())
            scan();
    };

    makePlaylistPanel(QStringLiteral("播放列表 1"), 1, &m_playlist1);
    makePlaylistPanel(QStringLiteral("播放列表 2"), 2, &m_playlist2);

    layout->addWidget(sectionTitle(QStringLiteral("文件夹收藏夹"), sidebar));
    m_favorites = new QListWidget(sidebar);
    m_favorites->setMaximumHeight(125);
    layout->addWidget(m_favorites);

    QSettings settings;
    const QStringList favorites = settings.value(QStringLiteral("favorites")).toStringList();
    for (const QString &folder : favorites)
        m_favorites->addItem(folder);
    if (m_favorites->count() == 0)
        m_favorites->addItem(QStringLiteral("暂无收藏文件夹"));

    connect(m_favorites, &QListWidget::itemDoubleClicked, this,
        [this](QListWidgetItem *item) {
            const QString folder = item->text();
            if (folder.isEmpty() || folder == QStringLiteral("暂无收藏文件夹"))
                return;

            QSettings currentSettings;
            currentSettings.setValue(QStringLiteral("playlist1/path"), folder);
            if (m_playlist1PathLabel)
                m_playlist1PathLabel->setText(
                    folder + QStringLiteral("  [已切换；不会自动扫描]"));
            restoreSnapshot(m_playlist1,
                currentSettings.value(QStringLiteral("playlist1/snapshot")).toStringList());
        });

    auto *hint = new QLabel(
        QStringLiteral("SMB：仅播放器启动时扫描一次\n打开列表不扫描；手动刷新可立即更新"),
        sidebar);
    hint->setStyleSheet(QStringLiteral("color:#7f8996; font-size:11px; padding-top:5px;"));
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

    m_videoSurface = new QWidget(videoFrame);
    m_videoSurface->setAttribute(Qt::WA_NativeWindow, true);
    auto *surfaceLayout = new QVBoxLayout(m_videoSurface);
    surfaceLayout->setContentsMargins(0, 0, 0, 0);
    m_videoPlaceholder = new QLabel(QStringLiteral("双击播放列表中的视频开始播放"), m_videoSurface);
    m_videoPlaceholder->setObjectName(QStringLiteral("videoPlaceholder"));
    m_videoPlaceholder->setAlignment(Qt::AlignCenter);
    surfaceLayout->addWidget(m_videoPlaceholder);
    videoLayout->addWidget(m_videoSurface);

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

    m_speedButton = new QPushButton(QStringLiteral("1.0×"), bar);
    m_speedButton->setToolTip(QStringLiteral("循环切换播放速度"));
    top->addWidget(m_speedButton);
    connect(m_speedButton, &QPushButton::clicked, this, [this]() {
        const QList<double> speeds = {1.0, 1.25, 1.5, 1.75, 2.0};
        const int index = speeds.indexOf(m_speed);
        m_speed = speeds.at((index + 1) % speeds.size());
        m_speedButton->setText(QStringLiteral("%1×").arg(m_speed, 0, 'f', 2));
        if (m_mpv && m_mpv->isReady()) m_mpv->setSpeed(m_speed);
    });
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
    m_playPauseButton = new QPushButton(QStringLiteral("播放/暂停"), bar);
    bottom->addWidget(m_playPauseButton);
    connect(m_playPauseButton, &QPushButton::clicked, this, [this]() {
        if (!m_mpv || !m_mpv->isReady()) return;
        m_paused = !m_paused;
        m_mpv->setPaused(m_paused);
        m_playPauseButton->setText(m_paused ? QStringLiteral("继续") : QStringLiteral("暂停"));
    });
    bottom->addWidget(iconButton(QStyle::SP_MediaSkipForward, QStringLiteral("下一个"), bar));
    bottom->addStretch();
    bottom->addWidget(new QLabel(QStringLiteral("音量"), bar));
    bottom->addWidget(iconButton(QStyle::SP_MediaVolume, QStringLiteral("音量"), bar));
    bottom->addWidget(iconButton(QStyle::SP_TitleBarMaxButton, QStringLiteral("全屏"), bar));
    layout->addLayout(bottom);
    return bar;
}

void MainWindow::playFile(const QString &path)
{
    QString error;
    if (!m_mpv->isReady() && !m_mpv->initialize(m_videoSurface->winId(), &error)) {
        QMessageBox::warning(this, QStringLiteral("播放器引擎不可用"), error);
        return;
    }
    if (!m_mpv->loadFile(path, &error)) {
        QMessageBox::warning(this, QStringLiteral("播放失败"), error);
        return;
    }
    m_paused = false;
    m_speed = 1.0;
    m_mpv->setSpeed(m_speed);
    m_speedButton->setText(QStringLiteral("1.0×"));
    m_currentFile->setText(QFileInfo(path).fileName());
    m_videoPlaceholder->hide();
    m_playPauseButton->setText(QStringLiteral("暂停"));
}
