#include "db/databasemanager.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QVariant>

DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::DatabaseManager() {
    m_userDb = QSqlDatabase::addDatabase("QSQLITE", "user_db");
    initUserDatabase();
}

DatabaseManager::~DatabaseManager() {
    if (m_db.isOpen()) {
        m_db.close();
    }
    if (m_userDb.isOpen()) {
        m_userDb.close();
    }
}

bool DatabaseManager::isValidNutraDatabase(const QSqlDatabase& db) {
    if (!db.isOpen()) return false;
    QSqlQuery query(db);
    // Check for a critical table, e.g., food_des
    return query.exec("SELECT 1 FROM food_des LIMIT 1");
}

bool DatabaseManager::connect(const QString& path) {
    if (m_db.isOpen()) {
        if (m_db.databaseName() == path) {
            return true;
        }
        m_db.close();
    }

    if (!QFileInfo::exists(path)) {
        qCritical() << "Database file not found:" << path;
        return false;
    }

    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(path);
    m_db.setConnectOptions("QSQLITE_OPEN_READONLY");

    if (!m_db.open()) {
        qCritical() << "Error opening database:" << m_db.lastError().text();
        return false;
    }

    if (!isValidNutraDatabase(m_db)) {
        qCritical() << "Invalid database: missing essential tables.";
        m_db.close();
        return false;
    }

    return true;
}

bool DatabaseManager::isOpen() const {
    return m_db.isOpen();
}

QSqlDatabase DatabaseManager::database() const {
    return m_db;
}

QSqlDatabase DatabaseManager::userDatabase() const {
    return m_userDb;
}

DatabaseManager::DatabaseInfo DatabaseManager::getDatabaseInfo(const QString& path) {
    DatabaseInfo info{false, "Unknown", 0};

    if (!QFileInfo::exists(path)) return info;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "info_connection");
        db.setDatabaseName(path);
        if (db.open()) {
            QSqlQuery query(db);

            // Get Version
            info.version = instance().getSchemaVersion(db);

            // Get App ID
            int appId = 0;
            if (query.exec("PRAGMA application_id") && query.next()) {
                appId = query.value(0).toInt();
            }

            // Determine Type
            if (appId == APP_ID_USDA) {
                info.type = "USDA";
                info.isValid = true;
            } else if (appId == APP_ID_USER) {
                info.type = "User";
                info.isValid = true;
            } else {
                // Fallback: Check tables
                bool hasFoodDes = query.exec("SELECT 1 FROM food_des LIMIT 1");
                bool hasLogFood = query.exec("SELECT 1 FROM log_food LIMIT 1");

                if (hasFoodDes) {
                    info.type = "USDA";
                    info.isValid = true;
                } else if (hasLogFood) {
                    info.type = "User";
                    info.isValid = true;
                }
            }

            db.close();
        }
    }
    QSqlDatabase::removeDatabase("info_connection");
    return info;
}

void DatabaseManager::initUserDatabase() {
    QString dirPath = QDir::homePath() + "/.nutra";
    QDir().mkpath(dirPath);
    QString path = dirPath + "/nt.sqlite3";
    m_userDb.setDatabaseName(path);

    if (!m_userDb.open()) {
        qCritical() << "Failed to open user database:" << m_userDb.lastError().text();
        return;
    }

    QSqlQuery query(m_userDb);

    // Check version
    int schemaVersionOnDisk = getSchemaVersion(m_userDb);

    qDebug() << "User database version:" << schemaVersionOnDisk;

    if (schemaVersionOnDisk == 0) {
        // Initialize from tables.sql
        QString schemaPath = QDir::currentPath() + "/lib/ntsqlite/sql/tables.sql";
        if (!QFileInfo::exists(schemaPath)) {
            // Fallback for installed location
            QString fallbackPath = "/usr/share/nutra/sql/tables.sql";
            if (QFileInfo::exists(fallbackPath)) {
                schemaPath = fallbackPath;
            } else {
                qCritical() << "Schema file not found at:" << schemaPath << "or" << fallbackPath;
                return;
            }
        }
        applySchema(query, schemaPath);
    }
}

void DatabaseManager::applySchema(QSqlQuery& query, const QString& schemaPath) {
    if (!QFileInfo::exists(schemaPath)) {
        qCritical() << "applySchema: Schema file does not exist:" << schemaPath;
        return;
    }
    QFile schemaFile(schemaPath);
    if (!schemaFile.open(QIODevice::ReadOnly)) {
        qCritical() << "Could not open schema file:" << schemaPath;
        return;
    }

    QTextStream in(&schemaFile);
    QString sql = in.readAll();

    // Allow for simple splitting for now as tables.sql is simple
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QStringList statements = sql.split(';', Qt::SkipEmptyParts);
#else
    QStringList statements = sql.split(';', QString::SkipEmptyParts);
#endif
    for (const QString& stmt : statements) {
        QString trimmed = stmt.trimmed();
        if (!trimmed.isEmpty() && !trimmed.startsWith("--")) {
            if (!query.exec(trimmed)) {
                qWarning() << "Schema init warning:" << query.lastError().text()
                           << "\nStmt:" << trimmed;
            }
        }
    }
    // Ensure version and ID are set
    if (!query.exec(QString("PRAGMA user_version = %1").arg(USER_SCHEMA_VERSION))) {
        qCritical() << "Failed to set user_version:" << query.lastError().text();
    }
    if (!query.exec(QString("PRAGMA application_id = %1").arg(APP_ID_USER))) {
        qCritical() << "Failed to set application_id:" << query.lastError().text();
    }
    qDebug() << "Upgraded user database version to" << USER_SCHEMA_VERSION << "and set App ID.";

    // --- Seeding Data ---

    // Ensure default profile exists
    query.exec("INSERT OR IGNORE INTO profile (id, name) VALUES (1, 'default')");

    // Seed standard meal names if table is empty
    query.exec("SELECT count(*) FROM meal_name");
    if (query.next() && query.value(0).toInt() == 0) {
        QStringList meals = {"Breakfast", "Lunch", "Dinner", "Snack", "Brunch"};
        for (const auto& meal : meals) {
            query.prepare("INSERT INTO meal_name (name) VALUES (?)");
            query.addBindValue(meal);
            query.exec();
        }
    }
}

int DatabaseManager::getSchemaVersion(const QSqlDatabase& db) {
    if (!db.isOpen()) return 0;
    QSqlQuery query(db);
    if (query.exec("PRAGMA user_version") && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}
