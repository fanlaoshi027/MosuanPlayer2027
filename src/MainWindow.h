#pragma once

#include <QMainWindow>

class QListWidget;
class QLabel;
class QPushButton;
class QSlider;

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

    QListWidget *m_playlist1 = nullptr;
    QListWidget *m_playlist2 = nullptr;
    QListWidget *m_favorites = nullptr;
    QLabel *m_playlist1PathLabel = nullptr;
    QLabel *m_playlist2PathLabel = nullptr;
    QPushButton *m_playlist1RefreshButton = nullptr;
    QPushButton *m_playlist2RefreshButton = nullptr;
    QLabel *m_videoPlaceholder = nullptr;
    QLabel *m_currentFile = nullptr;
    QSlider *m_progress = nullptr;
};
