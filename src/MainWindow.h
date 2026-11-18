#pragma once

#include <QMainWindow>
#include <QString>

class QListWidget;
class QLabel;
class QPushButton;
class QSlider;
class QWidget;
class MpvEngine;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void buildUi();
    QWidget *createSidebar();
    QWidget *createVideoArea();
    QWidget *createControlBar();
    void playFile(const QString &path);

    QListWidget *m_playlist1 = nullptr;
    QListWidget *m_playlist2 = nullptr;
    QListWidget *m_favorites = nullptr;
    QLabel *m_playlist1PathLabel = nullptr;
    QLabel *m_playlist2PathLabel = nullptr;
    QPushButton *m_playlist1RefreshButton = nullptr;
    QPushButton *m_playlist2RefreshButton = nullptr;
    QLabel *m_videoPlaceholder = nullptr;
    QWidget *m_videoSurface = nullptr;
    QPushButton *m_playPauseButton = nullptr;
    QPushButton *m_speedButton = nullptr;
    MpvEngine *m_mpv = nullptr;
    bool m_paused = false;
    double m_speed = 1.0;
    QLabel *m_currentFile = nullptr;
    QSlider *m_progress = nullptr;
};
