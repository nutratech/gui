#include "db/databasemanager.h"
#include "mainwindow.h"
#include <QApplication>
#include <QDebug>
#include <QDir>
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
    // If not found, default to XDG AppData location for error message/setup
    // But we can't create it here.
    dbPath = QDir::homePath() + "/.nutra/usda.sqlite3"; // Fallback default
    qWarning() << "Database not found in standard locations.";
  }

  if (!DatabaseManager::instance().connect(dbPath)) {
    QString errorMsg =
        QString("Failed to connect to database at:\n%1\n\nPlease ensure the "
                "database file exists or reinstall the application.")
            .arg(dbPath);
    qCritical() << errorMsg;
    QMessageBox::critical(nullptr, "Database Error", errorMsg);
    return 1;
  }
  qDebug() << "Connected to database at:" << dbPath;

  MainWindow window;
  window.show();

  return QApplication::exec();
}
