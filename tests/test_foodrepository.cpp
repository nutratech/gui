#include <QDir>
#include <QFileInfo>
#include <QtTest>

#include "db/databasemanager.h"
#include "db/foodrepository.h"

class TestFoodRepository : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // Setup DB connection
        // Allow override via environment variable for CI/testing
        QString envPath = qgetenv("NUTRA_DB_PATH");
        QString dbPath = envPath.isEmpty() ? QDir::homePath() + "/.nutra/usda.sqlite3" : envPath;

        if (!QFileInfo::exists(dbPath)) {
            QSKIP(
                "Database file not found (NUTRA_DB_PATH or ~/.nutra/usda.sqlite3). "
                "Skipping DB tests.");
        }

        bool connected = DatabaseManager::instance().connect(dbPath);
        QVERIFY2(connected, "Failed to connect to database");
    }

    void testSearchFoods() {
        FoodRepository repo;
        auto results = repo.searchFoods("apple");
        QVERIFY2(!results.empty(), "Search should return results for 'apple'");
        bool found = false;
        for (const auto& item : results) {
            if (item.description.contains("Apple", Qt::CaseInsensitive)) {
                found = true;
                break;
            }
        }
        QVERIFY2(found, "Search results should contain 'Apple'");
    }

    void testGetFoodNutrients() {
        FoodRepository repo;
        // Known ID for "Apples, raw, with skin" might be 9003 in SR28, but let's
        // search first or pick a known one if we knew it. Let's just use the first
        // result from search.
        auto results = repo.searchFoods("apple");
        if (results.empty()) QSKIP("No foods found to test nutrients");

        int foodId = results[0].id;
        auto nutrients = repo.getFoodNutrients(foodId);
        QVERIFY2(!nutrients.empty(), "Nutrients should not be empty for a valid food");
    }
};

QTEST_MAIN(TestFoodRepository)
#include "test_foodrepository.moc"
