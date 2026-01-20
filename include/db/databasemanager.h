#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>

class DatabaseManager {
public:
  static DatabaseManager &instance();
  bool connect(const QString &path);
  [[nodiscard]] bool isOpen() const;
  [[nodiscard]] QSqlDatabase database() const;

  DatabaseManager(const DatabaseManager &) = delete;
  DatabaseManager &operator=(const DatabaseManager &) = delete;

private:
  DatabaseManager();
  ~DatabaseManager();

  QSqlDatabase m_db;
};

#endif // DATABASEMANAGER_H
