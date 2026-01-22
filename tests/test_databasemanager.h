#ifndef TEST_DATABASEMANAGER_H
#define TEST_DATABASEMANAGER_H

#include <QObject>
#include <QtTest>

class TestDatabaseManager : public QObject {
    Q_OBJECT

private slots:
    void testUserDatabaseInit();
    void testInvalidDatabase();
};

#endif  // TEST_DATABASEMANAGER_H
