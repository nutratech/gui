#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>

class DatabaseManager {
public:
    static DatabaseManager& instance();
    bool connect(const QString& path);
    [[nodiscard]] bool isOpen() const;
    [[nodiscard]] QSqlDatabase database() const;
    [[nodiscard]] QSqlDatabase userDatabase() const;

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

private:
    DatabaseManager();
    ~DatabaseManager();

    void initUserDatabase();
    bool isValidNutraDatabase(const QSqlDatabase& db);

    QSqlDatabase m_db;
    QSqlDatabase m_userDb;
};

#endif  // DATABASEMANAGER_H
