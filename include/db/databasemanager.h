#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>

class DatabaseManager {
public:
    static DatabaseManager& instance();
    static constexpr int CURRENT_SCHEMA_VERSION = 9;
    bool connect(const QString& path);
    [[nodiscard]] bool isOpen() const;
    [[nodiscard]] QSqlDatabase database() const;
    [[nodiscard]] QSqlDatabase userDatabase() const;
    bool isValidNutraDatabase(const QSqlDatabase& db);

    struct DatabaseInfo {
        bool isValid;
        QString type;  // "USDA" or "User"
        int version;
    };

    DatabaseInfo getDatabaseInfo(const QString& path);

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

private:
    DatabaseManager();
    ~DatabaseManager();

    void initUserDatabase();

    QSqlDatabase m_db;
    QSqlDatabase m_userDb;
};

#endif  // DATABASEMANAGER_H
