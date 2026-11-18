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
#include <QTimer>
#include <QMouseEvent>
#include <QStyleOptionSlider>
#include <QMenu>
#include <QAction>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QColor>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>

#include <algorithm>

namespace {
class ClickableSlider final : public QSlider
{
public:
    explicit ClickableSlider(Qt::Orientation orientation, QWidget *parent = nullptr)
        : QSlider(orientation, parent) {}
protected:
    int valueAt(const QPoint &point) const
    {
        QStyleOptionSlider option;
        initStyleOption(&option);
        const QRect groove = style()->subControlRect(
            QStyle::CC_Slider, &option, QStyle::SC_SliderGroove, this);
        const QRect handle = style()->subControlRect(
            QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, this);
        const int sliderLength = orientation() == Qt::Horizontal
            ? handle.width() : handle.height();
        const int sliderMin = orientation() == Qt::Horizontal
            ? groove.x() : groove.y();
        const int sliderMax = sliderMin
            + (orientation() == Qt::Horizontal ? groove.width() : groove.height())
            - sliderLength;
        const int position = orientation() == Qt::Horizontal
            ? point.x() - sliderLength / 2 : point.y() - sliderLength / 2;
        return QStyle::sliderValueFromPosition(
            minimum(), maximum(), position - sliderMin,
            qMax(1, sliderMax - sliderMin), option.upsideDown);
    }
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() != Qt::LeftButton) {
            QSlider::mousePressEvent(event);
            return;
        }
        setSliderPosition(valueAt(event->position().toPoint()));
        setSliderDown(true);
        emit sliderMoved(sliderPosition());
        event->accept();
    }
    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (isSliderDown()) {
            setSliderPosition(valueAt(event->position().toPoint()));
            emit sliderMoved(sliderPosition());
            event->accept();
            return;
        }
        QSlider::mouseMoveEvent(event);
    }
    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton && isSliderDown()) {
            setSliderPosition(valueAt(event->position().toPoint()));
            setValue(sliderPosition());
            setSliderDown(false);
            event->accept();
            return;
        }
        QSlider::mouseReleaseEvent(event);
    }
};
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
    m_statusTimer = new QTimer(this);
    m_statusTimer->setInterval(250);
    connect(m_statusTimer, &QTimer::timeout, this, &MainWindow::updatePlaybackStatus);
    m_statusTimer->start();
    QSettings settings;
    m_invertEnabled = settings.value(QStringLiteral("video/invertEnabled"), false).toBool();
    m_invertBackground = settings.value(QStringLiteral("video/invertBackground"), QStringLiteral("#191919")).toString();
    m_cropZoom = settings.value(QStringLiteral("video/cropZoom"), 0.0).toDouble();
    m_cropPanX = settings.value(QStringLiteral("video/cropPanX"), 0.0).toDouble();
    m_cropPanY = settings.value(QStringLiteral("video/cropPanY"), 0.0).toDouble();
    if (m_invertButton)
        m_invertButton->setText(m_invertEnabled ? QStringLiteral("反色：开") : QStringLiteral("反色：关"));
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
        auto *titleLabel = sectionTitle(title, sidebar);
        titleLabel->setContextMenuPolicy(Qt::CustomContextMenu);
        layout->addWidget(titleLabel);

        auto *pathLabel = new QLabel(QStringLiteral("尚未指定文件夹"), sidebar);
        pathLabel->setObjectName(QStringLiteral("pathLabel"));
        pathLabel->setWordWrap(true);
        layout->addWidget(pathLabel);

        auto *buttons = new QHBoxLayout;
        auto *choose = new QPushButton(QStringLiteral("选择文件夹"), sidebar);
        auto *favorite = new QPushButton(QStringLiteral("收藏"), sidebar);
        buttons->addWidget(choose);
        buttons->addWidget(favorite);
        layout->addLayout(buttons);

        auto *list = new QListWidget(sidebar);
        list->setMinimumHeight(95);
        layout->addWidget(list, 1);
        *outList = list;

        if (index == 1)
            m_playlist1PathLabel = pathLabel;
        else
            m_playlist2PathLabel = pathLabel;

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
        auto scan = [this, index, list, pathLabel]() {
            QSettings currentSettings;
            const QString folder = currentSettings.value(
                QStringLiteral("playlist%1/path").arg(index)).toString();
            if (folder.isEmpty()) {
                pathLabel->setText(QStringLiteral("尚未指定文件夹"));
                return;
            }

            pathLabel->setText(folder + QStringLiteral("  [正在刷新…]"));

            auto *watcher = new QFutureWatcher<FolderScanResult>(this);
            connect(watcher, &QFutureWatcher<FolderScanResult>::finished, this,
                [watcher, index, list, pathLabel, folder]() {
                    const FolderScanResult result = watcher->result();
                    watcher->deleteLater();

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

        // Refresh is intentionally available from the playlist title context menu only.
        connect(titleLabel, &QWidget::customContextMenuRequested, this,
            [titleLabel, scan](const QPoint &position) {
                QMenu menu(titleLabel);
                QAction *refreshAction = menu.addAction(QStringLiteral("刷新此播放列表"));
                if (QAction *chosen = menu.exec(titleLabel->mapToGlobal(position)); chosen == refreshAction)
                    scan();
            });

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
            [this, list](QListWidgetItem *item) {
                const QString file = item->data(Qt::UserRole).toString();
                if (file.isEmpty())
                    return;
                m_activePlaylist = list;
                m_activeRow = list->row(item);
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
    m_invertButton = new QPushButton(m_invertEnabled ? QStringLiteral("反色：开") : QStringLiteral("反色：关"), bar);
    m_invertButton->setToolTip(QStringLiteral("开启或关闭 GPU 反色"));
    top->addWidget(m_invertButton);
    connect(m_invertButton, &QPushButton::clicked, this, [this]() {
        m_invertEnabled = !m_invertEnabled;
        QSettings settings;
        settings.setValue(QStringLiteral("video/invertEnabled"), m_invertEnabled);
        m_invertButton->setText(m_invertEnabled ? QStringLiteral("反色：开") : QStringLiteral("反色：关"));
        applyInvertShader();
    });

    auto *backgroundButton = new QPushButton(QStringLiteral("背景色"), bar);
    auto *backgroundMenu = new QMenu(backgroundButton);
    const QList<QPair<QString, QString>> backgrounds = {
        {QStringLiteral("90%灰黑"), QStringLiteral("#191919")},
        {QStringLiteral("焦炭"), QStringLiteral("#252525")},
        {QStringLiteral("深蓝"), QStringLiteral("#10243A")},
        {QStringLiteral("深绿"), QStringLiteral("#102E25")}
    };
    for (const auto &entry : backgrounds) {
        QAction *action = backgroundMenu->addAction(entry.first);
        action->setData(entry.second);
        connect(action, &QAction::triggered, this, [this, action]() {
            m_invertBackground = action->data().toString();
            QSettings settings;
            settings.setValue(QStringLiteral("video/invertBackground"), m_invertBackground);
            applyInvertShader();
        });
    }
    backgroundButton->setMenu(backgroundMenu);
    top->addWidget(backgroundButton);
    auto *crop = new QPushButton(QStringLiteral("画面裁切"), bar);
    crop->setToolTip(QStringLiteral("调整画面放大比例及可视区域位置"));
    top->addWidget(crop);
    connect(crop, &QPushButton::clicked, this, [this]() {
        if (!m_mpv || !m_mpv->isReady()) {
            QMessageBox::information(this, QStringLiteral("画面裁切"),
                QStringLiteral("请先开始播放视频，再调整画面。"));
            return;
        }
        QDialog dialog(this);
        dialog.setWindowTitle(QStringLiteral("画面裁切与定位"));
        dialog.setMinimumWidth(360);
        auto *form = new QFormLayout(&dialog);
        auto *zoom = new QSlider(Qt::Horizontal, &dialog);
        zoom->setRange(0, 20);
        zoom->setValue(qBound(0, qRound(m_cropZoom * 10.0), 20));
        auto *horizontal = new QSlider(Qt::Horizontal, &dialog);
        horizontal->setRange(-100, 100);
        horizontal->setValue(qBound(-100, qRound(m_cropPanX * 100.0), 100));
        auto *vertical = new QSlider(Qt::Horizontal, &dialog);
        vertical->setRange(-100, 100);
        vertical->setValue(qBound(-100, qRound(m_cropPanY * 100.0), 100));
        form->addRow(QStringLiteral("放大程度"), zoom);
        form->addRow(QStringLiteral("水平位置"), horizontal);
        form->addRow(QStringLiteral("垂直位置"), vertical);
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Reset | QDialogButtonBox::Close, &dialog);
        form->addRow(buttons);
        connect(zoom, &QSlider::valueChanged, this, [this](int v) {
            m_cropZoom = v / 10.0;
            m_mpv->setPropertyString("video-zoom", QString::number(m_cropZoom, 'f', 2));
            QSettings().setValue(QStringLiteral("video/cropZoom"), m_cropZoom);
        });
        connect(horizontal, &QSlider::valueChanged, this, [this](int v) {
            m_cropPanX = v / 100.0;
            m_mpv->setPropertyString("video-pan-x", QString::number(m_cropPanX, 'f', 2));
            QSettings().setValue(QStringLiteral("video/cropPanX"), m_cropPanX);
        });
        connect(vertical, &QSlider::valueChanged, this, [this](int v) {
            m_cropPanY = v / 100.0;
            m_mpv->setPropertyString("video-pan-y", QString::number(m_cropPanY, 'f', 2));
            QSettings().setValue(QStringLiteral("video/cropPanY"), m_cropPanY);
        });
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons->button(QDialogButtonBox::Reset), &QPushButton::clicked, &dialog, [&, this]() {
            zoom->setValue(0);
            horizontal->setValue(0);
            vertical->setValue(0);
        });
        dialog.exec();
    });
    layout->addLayout(top);

    auto *timeRow = new QHBoxLayout;
    m_timeLabel = new QLabel(QStringLiteral("00:00 / 00:00"), bar);
    m_timeLabel->setMinimumWidth(110);
    timeRow->addWidget(m_timeLabel);
    m_progress = new ClickableSlider(Qt::Horizontal, bar);
    m_progress->setRange(0, 1000);
    m_progress->setTracking(true);
    timeRow->addWidget(m_progress, 1);
    layout->addLayout(timeRow);
    connect(m_progress, &QSlider::sliderPressed, this, [this]() { m_seeking = true; });
    connect(m_progress, &QSlider::sliderReleased, this, [this]() {
        if (m_mpv && m_mpv->isReady()) {
            double duration = 0.0;
            if (m_mpv->getDoubleProperty("duration", &duration) && duration > 0.0)
                m_mpv->seek(duration * m_progress->value() / 1000.0);
        }
        m_seeking = false;
    });

    auto *bottom = new QHBoxLayout;
    auto *previousButton = iconButton(QStyle::SP_MediaSkipBackward, QStringLiteral("上一个"), bar);
    bottom->addWidget(previousButton);
    connect(previousButton, &QToolButton::clicked, this, [this]() { playAdjacent(-1); });
    m_playPauseButton = new QPushButton(QStringLiteral("播放/暂停"), bar);
    bottom->addWidget(m_playPauseButton);
    connect(m_playPauseButton, &QPushButton::clicked, this, [this]() {
        if (!m_mpv || !m_mpv->isReady()) return;
        m_paused = !m_paused;
        m_mpv->setPaused(m_paused);
        m_playPauseButton->setText(m_paused ? QStringLiteral("继续") : QStringLiteral("暂停"));
    });
    auto *nextButton = iconButton(QStyle::SP_MediaSkipForward, QStringLiteral("下一个"), bar);
    bottom->addWidget(nextButton);
    connect(nextButton, &QToolButton::clicked, this, [this]() { playAdjacent(1); });
    bottom->addStretch();
    bottom->addWidget(new QLabel(QStringLiteral("音量"), bar));
    m_volumeSlider = new QSlider(Qt::Horizontal, bar);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(80);
    m_volumeSlider->setFixedWidth(110);
    bottom->addWidget(m_volumeSlider);
    connect(m_volumeSlider, &QSlider::valueChanged, this, [this](int value) {
        if (m_mpv && m_mpv->isReady()) m_mpv->setVolume(value);
    });
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
    m_mpv->setVolume(m_volumeSlider ? m_volumeSlider->value() : 80);
    m_mpv->setPropertyString("video-zoom", QString::number(m_cropZoom, 'f', 2));
    m_mpv->setPropertyString("video-pan-x", QString::number(m_cropPanX, 'f', 2));
    m_mpv->setPropertyString("video-pan-y", QString::number(m_cropPanY, 'f', 2));
    if (m_invertEnabled) applyInvertShader();
    m_autoAdvanceTriggered = false;
    m_speedButton->setText(QStringLiteral("1.0×"));
    m_currentFile->setText(QFileInfo(path).fileName());
    m_videoPlaceholder->hide();
    m_playPauseButton->setText(QStringLiteral("暂停"));
}

QString MainWindow::formatTime(double seconds) const
{
    const qint64 total = qMax<qint64>(0, static_cast<qint64>(seconds));
    const qint64 hours = total / 3600;
    const qint64 minutes = (total % 3600) / 60;
    const qint64 secs = total % 60;
    if (hours > 0)
        return QStringLiteral("%1:%2:%3").arg(hours).arg(minutes, 2, 10, QLatin1Char('0')).arg(secs, 2, 10, QLatin1Char('0'));
    return QStringLiteral("%1:%2").arg(minutes).arg(secs, 2, 10, QLatin1Char('0'));
}

void MainWindow::updatePlaybackStatus()
{
    if (!m_mpv || !m_mpv->isReady()) return;
    double position = 0.0;
    double duration = 0.0;
    const bool hasPosition = m_mpv->getDoubleProperty("time-pos", &position);
    const bool hasDuration = m_mpv->getDoubleProperty("duration", &duration);
    if (!hasPosition || !hasDuration || duration <= 0.0) return;

    if (!m_seeking) {
        const int value = qBound(0, static_cast<int>((position / duration) * 1000.0), 1000);
        m_progress->setValue(value);
    }
    m_timeLabel->setText(formatTime(position) + QStringLiteral(" / ") + formatTime(duration));

    if (!m_paused && position >= duration - 0.45 && !m_autoAdvanceTriggered) {
        m_autoAdvanceTriggered = true;
        playAdjacent(1);
    }
    if (position < duration - 1.0)
        m_autoAdvanceTriggered = false;
}

void MainWindow::playAdjacent(int offset)
{
    if (!m_activePlaylist || m_activePlaylist->count() == 0) return;
    const int nextRow = m_activeRow + offset;
    if (nextRow < 0 || nextRow >= m_activePlaylist->count()) return;
    auto *item = m_activePlaylist->item(nextRow);
    const QString file = item ? item->data(Qt::UserRole).toString() : QString();
    if (file.isEmpty()) return;
    m_activeRow = nextRow;
    m_activePlaylist->setCurrentRow(nextRow);
    playFile(file);
}

void MainWindow::applyInvertShader()
{
    if (!m_mpv || !m_mpv->isReady()) return;
    if (!m_invertEnabled) {
        if (!m_mpv->setShader(QString()))
            QMessageBox::warning(this, QStringLiteral("反色关闭失败"),
                QStringLiteral("mpv 未能清除反色 Shader。"));
        return;
    }

    const QColor bg(m_invertBackground);
    if (!bg.isValid()) return;
    const QString shader = QStringLiteral(
        "//!HOOK OUTPUT\n"
        "//!BIND HOOKED\n"
        "//!DESC Mosuan simple GPU inversion\n"
        "vec4 hook() {\n"
        "    vec4 color = HOOKED_tex(HOOKED_pos);\n"
        "    float whiteMask = smoothstep(0.90, 0.995, min(min(color.r, color.g), color.b));\n"
        "    vec3 inverted = vec3(1.0) - color.rgb;\n"
        "    vec3 background = vec3(%1, %2, %3);\n"
        "    color.rgb = mix(inverted, background, whiteMask);\n"
        "    return color;\n"
        "}\n")
        .arg(bg.redF(), 0, 'f', 6)
        .arg(bg.greenF(), 0, 'f', 6)
        .arg(bg.blueF(), 0, 'f', 6);

    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (appData.isEmpty() || !QDir().mkpath(appData)) return;
    const QString shaderPath = QDir(appData).filePath(QStringLiteral("mosuan-invert.glsl"));
    QFile file(shaderPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) return;
    file.write(shader.toUtf8());
    file.close();
    if (!m_mpv->setShader(shaderPath)) {
        QMessageBox::warning(this, QStringLiteral("反色启用失败"),
            QStringLiteral("mpv 未能应用反色 Shader。请检查当前视频输出是否支持 GPU Shader。"));
        m_invertEnabled = false;
        QSettings settings;
        settings.setValue(QStringLiteral("video/invertEnabled"), false);
        if (m_invertButton)
            m_invertButton->setText(QStringLiteral("反色：关"));
    }
}
