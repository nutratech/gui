#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QtTest>

#include "db/databasemanager.h"

class TestDatabaseManager : public QObject {
    Q_OBJECT

private slots:
    void testUserDatabaseInit() {
        // Use a temporary database path
        QString dbPath = QDir::tempPath() + "/nutra_test_db.sqlite3";
        if (QFileInfo::exists(dbPath)) {
            QFile::remove(dbPath);
        }

        // We can't easily instruct DatabaseManager to use a specific path for userDatabase()
        // without modifying it to accept a path injection or using a mock.
        // However, `DatabaseManager::connect` allows opening arbitrary databases.

        // Let's test the validity check on a fresh DB.

        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "test_connection");
        db.setDatabaseName(dbPath);
        QVERIFY(db.open());

        // Initialize schema manually (simulating initUserDatabase behavior if we can't invoke it
        // directly) OR, verify the one in ~/.nutra if we want integration test. Let's assume we
        // want to verify the logic in DatabaseManager::getDatabaseInfo which requires a db on disk.

        // Let's create a minimal valid user DB
        QSqlQuery q(db);
        q.exec("PRAGMA application_id = 1314145346");  // 'NTDB'
        q.exec("PRAGMA user_version = 9");
        q.exec("CREATE TABLE log_food (id int)");

        db.close();

        auto info = DatabaseManager::instance().getDatabaseInfo(dbPath);
        QCOMPARE(info.type, QString("User"));
        QVERIFY(info.isValid);
        QCOMPARE(info.version, 9);

        QFile::remove(dbPath);
    }

    void testInvalidDatabase() {
        QString dbPath = QDir::tempPath() + "/nutra_invalid.sqlite3";
        if (QFileInfo::exists(dbPath)) QFile::remove(dbPath);

        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "invalid_conn");
        db.setDatabaseName(dbPath);
        QVERIFY(db.open());
        // Empty DB
        db.close();

        auto info = DatabaseManager::instance().getDatabaseInfo(dbPath);
        QVERIFY(info.isValid == false);

        QFile::remove(dbPath);
    }
};

QTEST_MAIN(TestDatabaseManager)
#include "test_databasemanager.moc"
