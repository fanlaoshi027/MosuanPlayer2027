#pragma once

#include <QMainWindow>
#include <QString>

class QListWidget;
class QLabel;
class QPushButton;
class QSlider;
class QWidget;
class QTimer;
class QMenu;
class MpvEngine;
class QKeyEvent;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
protected:
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    void buildUi();
    QWidget *createSidebar();
    QWidget *createVideoArea();
    QWidget *createControlBar();
    void playFile(const QString &path);
    void updatePlaybackStatus();
    void playAdjacent(int offset);
    QString formatTime(double seconds) const;
    void applyInvertShader();
    bool applyVideoCrop(QString *errorMessage = nullptr);
    void saveVideoCrop();
    void populateFavorites();
    QString resumeSettingsKey(const QString &path) const;
    void saveResumePosition(bool force = false);

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
    QSlider *m_volumeSlider = nullptr;
    QLabel *m_timeLabel = nullptr;
    QTimer *m_statusTimer = nullptr;
    QListWidget *m_activePlaylist = nullptr;
    int m_activeRow = -1;
    bool m_seeking = false;
    bool m_autoAdvanceTriggered = false;
    int m_cropLeft = 0;
    int m_cropRight = 0;
    int m_cropTop = 0;
    int m_cropBottom = 0;
    bool m_cropApplyPending = false;
    QString m_currentPath;
    bool m_resumeApplyPending = false;
    double m_resumePosition = 0.0;
    qint64 m_lastResumeSaveBucket = -1;
    bool m_invertEnabled = false;
    QString m_invertBackground = QStringLiteral("#191919");
    QPushButton *m_invertButton = nullptr;
};
