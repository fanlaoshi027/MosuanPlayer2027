#include "MpvEngine.h"
#include <QByteArray>
#include <QFileInfo>

MpvEngine::MpvEngine(QObject *parent) : QObject(parent) {
#ifdef Q_OS_WIN
    m_library.setFileName(QStringLiteral("libmpv-2"));
#else
    m_library.setFileName(QStringLiteral("mpv"));
#endif
}
MpvEngine::~MpvEngine() {
    if (m_handle && m_terminateDestroy) {
        m_terminateDestroy(m_handle);
        m_handle = nullptr;
    }
    if (m_library.isLoaded()) m_library.unload();
}
bool MpvEngine::resolveApi(QString *errorMessage) {
    if (!m_library.load()) {
        if (errorMessage) *errorMessage = QStringLiteral("未找到 libmpv-2.dll。请将 LGPL 版 libmpv-2.dll 放在播放器程序目录。\n%1").arg(m_library.errorString());
        return false;
    }
    m_create = reinterpret_cast<CreateFn>(m_library.resolve("mpv_create"));
    m_setOptionString = reinterpret_cast<SetOptionStringFn>(m_library.resolve("mpv_set_option_string"));
    m_initialize = reinterpret_cast<InitializeFn>(m_library.resolve("mpv_initialize"));
    m_command = reinterpret_cast<CommandFn>(m_library.resolve("mpv_command"));
    m_setPropertyString = reinterpret_cast<SetPropertyStringFn>(m_library.resolve("mpv_set_property_string"));
    m_getProperty = reinterpret_cast<GetPropertyFn>(m_library.resolve("mpv_get_property"));
    m_terminateDestroy = reinterpret_cast<TerminateDestroyFn>(m_library.resolve("mpv_terminate_destroy"));
    if (!m_create || !m_setOptionString || !m_initialize || !m_command || !m_setPropertyString || !m_getProperty || !m_terminateDestroy) {
        if (errorMessage) *errorMessage = QStringLiteral("libmpv-2.dll 缺少必要的 C API 导出函数。");
        m_library.unload();
        return false;
    }
    return true;
}
bool MpvEngine::initialize(WId nativeWindow, QString *errorMessage) {
    if (m_handle) return true;
    if (!resolveApi(errorMessage)) return false;
    m_handle = m_create();
    if (!m_handle) {
        if (errorMessage) *errorMessage = QStringLiteral("mpv_create 创建播放实例失败。");
        return false;
    }
    const QByteArray wid = QByteArray::number(static_cast<qulonglong>(reinterpret_cast<quintptr>(nativeWindow)));
    const auto option = [this](const char *key, const char *value) {
        return m_setOptionString(m_handle, key, value) >= 0;
    };
    const bool ok = option("wid", wid.constData())
        && option("vo", "gpu")
        && option("hwdec", "auto-safe")
        && option("terminal", "no")
        && option("osc", "no")
        && option("input-default-bindings", "no")
        && option("input-vo-keyboard", "no")
        && option("keep-open", "yes");
    if (!ok || m_initialize(m_handle) < 0) {
        if (errorMessage) *errorMessage = QStringLiteral("libmpv 初始化失败，请检查 DLL 版本及依赖。");
        m_terminateDestroy(m_handle);
        m_handle = nullptr;
        return false;
    }
    return true;
}
bool MpvEngine::loadFile(const QString &path, QString *errorMessage) {
    if (!m_handle) {
        if (errorMessage) *errorMessage = QStringLiteral("播放引擎尚未初始化。");
        return false;
    }
    const QByteArray utf8 = QFileInfo(path).absoluteFilePath().toUtf8();
    const char *args[] = {"loadfile", utf8.constData(), "replace", nullptr};
    const int rc = m_command(m_handle, args);
    if (rc < 0 && errorMessage) *errorMessage = QStringLiteral("libmpv 无法打开文件：%1").arg(path);
    return rc >= 0;
}
bool MpvEngine::setPaused(bool paused) {
    return m_handle && m_setPropertyString(m_handle, "pause", paused ? "yes" : "no") >= 0;
}
bool MpvEngine::setSpeed(double speed) {
    if (!m_handle) return false;
    const QByteArray value = QByteArray::number(speed, 'f', 2);
    return m_setPropertyString(m_handle, "speed", value.constData()) >= 0;
}

bool MpvEngine::setVolume(double volume) {
    if (!m_handle) return false;
    const QByteArray value = QByteArray::number(qBound(0.0, volume, 100.0), 'f', 1);
    return m_setPropertyString(m_handle, "volume", value.constData()) >= 0;
}
bool MpvEngine::seek(double seconds) {
    if (!m_handle) return false;
    const QByteArray value = QByteArray::number(qMax(0.0, seconds), 'f', 3);
    const char *args[] = {"seek", value.constData(), "absolute", nullptr};
    return m_command(m_handle, args) >= 0;
}
bool MpvEngine::getDoubleProperty(const char *name, double *value) const {
    if (!m_handle || !m_getProperty || !value) return false;
    // MPV_FORMAT_DOUBLE is 5 in libmpv's public C API.
    return m_getProperty(m_handle, name, 5, value) >= 0;
}

bool MpvEngine::setShader(const QString &shaderPath) {
    if (!m_handle) return false;
    if (shaderPath.isEmpty()) {
        const char *args[] = {"change-list", "glsl-shaders", "clr", nullptr};
        return m_command(m_handle, args) >= 0;
    }
    const QByteArray path = shaderPath.toUtf8();
    const char *args[] = {"change-list", "glsl-shaders", "set", path.constData(), nullptr};
    return m_command(m_handle, args) >= 0;
}

bool MpvEngine::setPropertyString(const char *name, const QString &value) {
    if (!m_handle || !name) return false;
    const QByteArray utf8 = value.toUtf8();
    return m_setPropertyString(m_handle, name, utf8.constData()) >= 0;
}

bool MpvEngine::getInt64Property(const char *name, qint64 *value) const {
    if (!m_handle || !m_getProperty || !value) return false;
    return m_getProperty(m_handle, name, 4, value) >= 0;
}
