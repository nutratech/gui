#include "utils/pythonservicemanager.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QStandardPaths>

PythonServiceManager& PythonServiceManager::instance() {
    static PythonServiceManager instance;
    return instance;
}

PythonServiceManager::PythonServiceManager(QObject* parent) : QObject(parent) {
    m_network = new QNetworkAccessManager(this);

    // Load settings
    QSettings settings;
    m_enabled = settings.value(SETTING_ENABLED, false).toBool();
    m_port = settings.value(SETTING_PORT, 5001).toInt();

    findPythonPath();

    // Auto-start if enabled
    if (m_enabled) {
        startService();
    }
}

PythonServiceManager::~PythonServiceManager() {
    stopService();
}

bool PythonServiceManager::isEnabled() const {
    return m_enabled;
}

void PythonServiceManager::setEnabled(bool enabled) {
    if (m_enabled == enabled) return;

    m_enabled = enabled;
    QSettings settings;
    settings.setValue(SETTING_ENABLED, enabled);

    if (enabled) {
        startService();
    } else {
        stopService();
    }
}

bool PythonServiceManager::isRunning() const {
    return m_process != nullptr && m_process->state() == QProcess::Running;
}

void PythonServiceManager::findPythonPath() {
    // Try common Python paths
    QStringList candidates = {
        "python3",
        "python",
#ifdef Q_OS_WIN
        "py",
#endif
    };

    for (const QString& candidate : candidates) {
        QString path = QStandardPaths::findExecutable(candidate);
        if (!path.isEmpty()) {
            m_pythonPath = path;
            qDebug() << "Found Python at:" << m_pythonPath;
            return;
        }
    }

    qWarning() << "Python not found in PATH";
}

void PythonServiceManager::startService() {
    if (isRunning()) {
        qDebug() << "Python service already running";
        return;
    }

    if (m_pythonPath.isEmpty()) {
        emit parseError("Python not found. Install Python 3 to use NLP features.");
        return;
    }

    if (m_process != nullptr) {
        m_process->deleteLater();
    }

    m_process = new QProcess(this);

    // Find the pylang_serv module
    QString modulePath;

    // Check relative to app (development)
    QString devPath = QCoreApplication::applicationDirPath() + "/../lib/pylang_serv";
    if (QDir(devPath).exists()) {
        modulePath = devPath;
    }

    // Check installed location
    QString installPath = "/usr/share/nutra/pylang_serv";
    if (modulePath.isEmpty() && QDir(installPath).exists()) {
        modulePath = installPath;
    }

    // Check user local
    QString userPath = QDir::homePath() + "/.local/share/nutra/pylang_serv";
    if (modulePath.isEmpty() && QDir(userPath).exists()) {
        modulePath = userPath;
    }

    if (modulePath.isEmpty()) {
        emit parseError("Python NLP module not found. Ensure pylang_serv is installed.");
        return;
    }

    connect(m_process, &QProcess::started, this, [this]() {
        qDebug() << "Python NLP service started on port" << m_port;
        emit serviceStarted();
    });

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this](int exitCode, QProcess::ExitStatus status) {
                qDebug() << "Python service stopped. Exit code:" << exitCode;
                emit serviceStopped();
            });

    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        qWarning() << "Python service error:" << error;
        emit parseError("Failed to start Python service");
    });

    // Start the server
    m_process->setWorkingDirectory(modulePath);
    m_process->start(m_pythonPath, {"-m", "pylang_serv.server"});
}

void PythonServiceManager::stopService() {
    if (m_process == nullptr) return;

    if (m_process->state() == QProcess::Running) {
        m_process->terminate();
        if (!m_process->waitForFinished(3000)) {
            m_process->kill();
        }
    }

    m_process->deleteLater();
    m_process = nullptr;
}

void PythonServiceManager::parseIngredient(const QString& text) {
    if (!isRunning()) {
        emit parseError("NLP service not running. Enable it in Settings.");
        return;
    }

    QUrl url(QString("http://127.0.0.1:%1/parse").arg(m_port));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["text"] = text;
    QByteArray data = QJsonDocument(json).toJson();

    QNetworkReply* reply = m_network->post(request, data);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit parseError(reply->errorString());
            return;
        }

        QByteArray responseData = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(responseData);

        if (doc.isNull() || !doc.isObject()) {
            emit parseError("Invalid response from NLP service");
            return;
        }

        emit parseComplete(doc.object());
    });
}
