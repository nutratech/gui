#include "db/databasemanager.h"
#include <QDebug>
#include <QFileInfo>
#include <QSqlError>

DatabaseManager &DatabaseManager::instance() {
  static DatabaseManager instance;
  return instance;
}

DatabaseManager::DatabaseManager() = default;

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
