#pragma once
#include <QObject>
#include <QString>
#include <QLibrary>
#include <QtGlobal>
#include <QtGui/qwindowdefs.h>

class MpvEngine final : public QObject {
    Q_OBJECT
public:
    explicit MpvEngine(QObject *parent = nullptr);
    ~MpvEngine() override;
    bool initialize(WId nativeWindow, QString *errorMessage = nullptr);
    bool loadFile(const QString &path, QString *errorMessage = nullptr);
    bool setPaused(bool paused);
    bool setSpeed(double speed);
    bool isReady() const noexcept { return m_handle != nullptr; }
private:
    using CreateFn = void *(*)();
    using SetOptionStringFn = int (*)(void *, const char *, const char *);
    using InitializeFn = int (*)(void *);
    using CommandFn = int (*)(void *, const char **);
    using SetPropertyStringFn = int (*)(void *, const char *, const char *);
    using TerminateDestroyFn = void (*)(void *);
    QLibrary m_library;
    void *m_handle = nullptr;
    CreateFn m_create = nullptr;
    SetOptionStringFn m_setOptionString = nullptr;
    InitializeFn m_initialize = nullptr;
    CommandFn m_command = nullptr;
    SetPropertyStringFn m_setPropertyString = nullptr;
    TerminateDestroyFn m_terminateDestroy = nullptr;
    bool resolveApi(QString *errorMessage);
};
