#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>

class DatabaseManager {
public:
    static DatabaseManager& instance();
    static constexpr int USER_SCHEMA_VERSION = 9;
    static constexpr int USDA_SCHEMA_VERSION = 1;   // Schema version for USDA data import
    static constexpr int APP_ID_USDA = 0x55534441;  // 'USDA' (ASCII)
    static constexpr int APP_ID_USER = 0x4E555452;  // 'NUTR' (ASCII)
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
    void applySchema(QSqlQuery& query, const QString& schemaPath);
    int getSchemaVersion(const QSqlDatabase& db);

    QSqlDatabase m_db;
    QSqlDatabase m_userDb;
};

#endif  // DATABASEMANAGER_H
