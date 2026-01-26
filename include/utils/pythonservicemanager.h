#ifndef PYTHONSERVICEMANAGER_H
#define PYTHONSERVICEMANAGER_H

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QProcess>
#include <QSettings>

/**
 * @brief Manages the optional Python NLP microservice.
 *
 * This is an optional feature that can be enabled/disabled in settings.
 * When enabled, spawns a Python Flask server for natural language
 * ingredient parsing.
 */
class PythonServiceManager : public QObject {
    Q_OBJECT

public:
    static PythonServiceManager& instance();

    [[nodiscard]] bool isEnabled() const;
    void setEnabled(bool enabled);

    [[nodiscard]] bool isRunning() const;

    /**
     * @brief Parse an ingredient string using NLP.
     * @param text Natural language ingredient (e.g., "2 cups flour")
     *
     * Emits parseComplete or parseError when done.
     */
    void parseIngredient(const QString& text);

signals:
    void serviceStarted();
    void serviceStopped();
    void parseComplete(const QJsonObject& result);
    void parseError(const QString& error);

public slots:
    void startService();
    void stopService();

private:
    explicit PythonServiceManager(QObject* parent = nullptr);
    ~PythonServiceManager() override;

    void findPythonPath();

    QProcess* m_process = nullptr;
    QNetworkAccessManager* m_network = nullptr;
    QString m_pythonPath;
    bool m_enabled = false;
    int m_port = 5001;

    static constexpr const char* SETTING_ENABLED = "nlp/enabled";
    static constexpr const char* SETTING_PORT = "nlp/port";
};

#endif  // PYTHONSERVICEMANAGER_H
