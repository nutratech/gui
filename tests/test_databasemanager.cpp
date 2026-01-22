#include "test_databasemanager.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>

#include "db/databasemanager.h"

void TestDatabaseManager::testUserDatabaseInit() {
    // Use a temporary database path
    QString dbPath = QDir::tempPath() + "/nutra_test_db.sqlite3";
    if (QFileInfo::exists(dbPath)) {
        QFile::remove(dbPath);
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "test_connection");
    db.setDatabaseName(dbPath);
    QVERIFY(db.open());

    // Initialize schema manually (simulating initUserDatabase behavior)
    QSqlQuery q(db);
    q.exec("PRAGMA application_id = 1314145346");  // 'NTDB'
    q.exec("PRAGMA user_version = 9");
    q.exec("CREATE TABLE log_food (id int)");

    db.close();

    auto info = DatabaseManager::instance().getDatabaseInfo(dbPath);
    QCOMPARE(info.type, QString("User"));
    QVERIFY(info.isValid);
    QCOMPARE(info.version, 9);

    QSqlDatabase::removeDatabase("test_connection");
    QFile::remove(dbPath);
}

void TestDatabaseManager::testInvalidDatabase() {
    QString dbPath = QDir::tempPath() + "/nutra_invalid.sqlite3";
    if (QFileInfo::exists(dbPath)) QFile::remove(dbPath);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "invalid_conn");
    db.setDatabaseName(dbPath);
    QVERIFY(db.open());
    // Empty DB
    db.close();

    auto info = DatabaseManager::instance().getDatabaseInfo(dbPath);
    QVERIFY(info.isValid == false);

    QSqlDatabase::removeDatabase("invalid_conn");
    QFile::remove(dbPath);
}

QTEST_MAIN(TestDatabaseManager)
