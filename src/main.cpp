#include "db/databasemanager.h"
#include "mainwindow.h"
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QMessageBox>
#include <QStandardPaths>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  QApplication::setApplicationName("Nutra");
  QApplication::setOrganizationName("NutraTech");
  QApplication::setWindowIcon(QIcon(":/resources/nutrition_icon-no_bg.png"));

  // Connect to database
  // Search order:
  // 1. Environment variable NUTRA_DB_PATH
  // 2. Local user data: ~/.local/share/nutra/usda.sqlite3
  // 3. System install: /usr/local/share/nutra/usda.sqlite3
  // 4. System install: /usr/share/nutra/usda.sqlite3
  // 5. Legacy: ~/.nutra/usda.sqlite3

  QStringList searchPaths;
  QString envPath = qEnvironmentVariable("NUTRA_DB_PATH");
  if (!envPath.isEmpty())
    searchPaths << envPath;

  searchPaths << QStandardPaths::locate(QStandardPaths::AppDataLocation,
                                        "usda.sqlite3",
                                        QStandardPaths::LocateFile);
  searchPaths << QDir::homePath() + "/.local/share/nutra/usda.sqlite3";
  searchPaths << "/usr/local/share/nutra/usda.sqlite3";
  searchPaths << "/usr/share/nutra/usda.sqlite3";
  searchPaths << QDir::homePath() + "/.nutra/usda.sqlite3";

  QString dbPath;
  for (const QString &path : searchPaths) {
    if (!path.isEmpty() && QFileInfo::exists(path)) {
      dbPath = path;
      break;
    }
  }

  if (dbPath.isEmpty()) {
    qWarning() << "Database not found in standard locations.";
    QMessageBox::StandardButton resBtn = QMessageBox::question(
        nullptr, "Database Not Found",
        "The Nutrient database (usda.sqlite3) was not found.\nWould you like "
        "to browse for it manually?",
        QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes);

    if (resBtn == QMessageBox::Yes) {
      dbPath = QFileDialog::getOpenFileName(
          nullptr, "Find usda.sqlite3", QDir::homePath(),
          "SQLite Databases (*.sqlite3 *.db)");
    }

    if (dbPath.isEmpty()) {
      return 1; // User cancelled or still not found
    }
  }

  if (!DatabaseManager::instance().connect(dbPath)) {
    QString errorMsg =
        QString("Failed to connect to database at:\n%1\n\nPlease ensure the "
                "database file exists or browse for a valid SQLite file.")
            .arg(dbPath);
    qCritical() << errorMsg;
    QMessageBox::critical(nullptr, "Database Error", errorMsg);
    // Let's try to let the user browse one more time before giving up
    return 1;
  }
  qDebug() << "Connected to database at:" << dbPath;

  MainWindow window;
  window.show();

  return QApplication::exec();
}
