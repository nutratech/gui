#include "db/databasemanager.h"
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QVariant>

DatabaseManager &DatabaseManager::instance() {
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
}

bool DatabaseManager::connect(const QString &path) {
  if (m_db.isOpen()) {
    return true;
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

  return true;
}

bool DatabaseManager::isOpen() const { return m_db.isOpen(); }

QSqlDatabase DatabaseManager::database() const { return m_db; }

QSqlDatabase DatabaseManager::userDatabase() const { return m_userDb; }

void DatabaseManager::initUserDatabase() {
  QString path = QDir::homePath() + "/.nutra/nt.sqlite3";
  m_userDb.setDatabaseName(path);

  if (!m_userDb.open()) {
    qCritical() << "Failed to open user database:"
                << m_userDb.lastError().text();
    return;
  }

  QSqlQuery query(m_userDb);
  // Create profile table (simplified version of CLI's schema)
  if (!query.exec("CREATE TABLE IF NOT EXISTS profile ("
                  "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                  "name TEXT UNIQUE NOT NULL)")) {
    qCritical() << "Failed to create profile table:"
                << query.lastError().text();
  }

  // Ensure default profile exists
  query.exec("INSERT OR IGNORE INTO profile (id, name) VALUES (1, 'default')");

  // Create rda table
  if (!query.exec("CREATE TABLE IF NOT EXISTS rda ("
                  "profile_id INTEGER NOT NULL, "
                  "nutr_id INTEGER NOT NULL, "
                  "rda REAL NOT NULL, "
                  "PRIMARY KEY (profile_id, nutr_id), "
                  "FOREIGN KEY (profile_id) REFERENCES profile (id))")) {
    qCritical() << "Failed to create rda table:" << query.lastError().text();
  }
}
