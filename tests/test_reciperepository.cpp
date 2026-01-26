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

        // Ensure we start with a fresh database
        // DatabaseManager uses ~/.nutra/nt.sqlite3 so we must clean it up
        // ONLY do this in CI to avoid wiping local dev data!
        if (!qEnvironmentVariable("CI").isEmpty()) {
            QString dbPath = QDir::homePath() + "/.nutra/nt.sqlite3";
            if (QFileInfo::exists(dbPath)) {
                QFile::remove(dbPath);
            }
        }
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
