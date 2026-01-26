#include <QDir>
#include <QFile>
#include <QtTest>

#include "db/databasemanager.h"
#include "db/reciperepository.h"

class TestRecipeRepository : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // Setup temporary DB and directory
        QStandardPaths::setTestModeEnabled(true);
        // Ensure user DB is open (in memory or temp file)
        // DatabaseManager singleton might need configuration
    }

    void testLoadCsv() {
        RecipeRepository repo;

        // Create dummy CSV
        QString recipeDir = QDir::tempPath() + "/nutra_test_recipes";
        QDir().mkpath(recipeDir);

        QFile file(recipeDir + "/test_recipe.csv");
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "Unit Test Recipe,Mix it up,1234,200\n";
            file.close();
        }

        repo.loadCsvRecipes(recipeDir);

        // Verify
        auto recipes = repo.getAllRecipes();
        bool found = false;
        for (const auto& r : recipes) {
            if (r.name == "Unit Test Recipe") {
                found = true;
                break;
            }
        }
        QVERIFY(found);
    }

    void cleanupTestCase() {
        QDir(QDir::tempPath() + "/nutra_test_recipes").removeRecursively();
    }
};

QTEST_MAIN(TestRecipeRepository)
#include "test_reciperepository.moc"
