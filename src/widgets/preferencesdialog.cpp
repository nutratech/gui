#include "widgets/preferencesdialog.h"

#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSqlQuery>
#include <QTabWidget>
#include <QVBoxLayout>

#include "db/databasemanager.h"

PreferencesDialog::PreferencesDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Preferences");
    setMinimumSize(450, 400);
    setupUi();
    loadStatistics();
}

void PreferencesDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    tabWidget = new QTabWidget(this);

    // === Usage Statistics Tab ===
    auto* statsWidget = new QWidget();
    auto* statsLayout = new QVBoxLayout(statsWidget);

    // --- Counts Group ---
    auto* countsGroup = new QGroupBox("Data Counts");
    auto* countsLayout = new QFormLayout(countsGroup);

    lblFoodLogs = new QLabel("--");
    lblCustomFoods = new QLabel("--");
    lblRdaOverrides = new QLabel("--");
    lblRecipes = new QLabel("--");
    lblSnapshots = new QLabel("--");

    countsLayout->addRow("Food Log Entries:", lblFoodLogs);
    countsLayout->addRow("Custom Foods:", lblCustomFoods);
    countsLayout->addRow("RDA Overrides:", lblRdaOverrides);
    countsLayout->addRow("Recipes:", lblRecipes);
    countsLayout->addRow("Snapshots/Backups:", lblSnapshots);

    // --- Sizes Group ---
    auto* sizesGroup = new QGroupBox("Database Sizes");
    auto* sizesLayout = new QFormLayout(sizesGroup);

    lblUsdaSize = new QLabel("--");
    lblUserSize = new QLabel("--");
    lblBackupSize = new QLabel("--");

    sizesLayout->addRow("USDA Database:", lblUsdaSize);
    sizesLayout->addRow("User Database:", lblUserSize);
    sizesLayout->addRow("Total Backup Size:", lblBackupSize);

    // --- Disclaimer ---
    auto* disclaimerLabel = new QLabel(
        "<b>Note:</b> The USDA database contains public domain data and is "
        "<b>read-only</b>. Only your personal user data (logs, custom foods, "
        "RDAs, recipes) is backed up.<br><br>"
        "<i>You are encouraged to periodically upload your snapshots to an "
        "online drive, archive them in a Git repository, or save them to a "
        "backup USB stick or external HDD.</i>");
    disclaimerLabel->setWordWrap(true);
    disclaimerLabel->setStyleSheet("color: #666; padding: 10px;");

    statsLayout->addWidget(countsGroup);
    statsLayout->addWidget(sizesGroup);
    statsLayout->addWidget(disclaimerLabel);
    statsLayout->addStretch();

    tabWidget->addTab(statsWidget, "Usage Statistics");

    mainLayout->addWidget(tabWidget);
}

void PreferencesDialog::loadStatistics() {
    QSqlDatabase userDb = DatabaseManager::instance().userDatabase();
    QSqlDatabase usdaDb = DatabaseManager::instance().database();

    // --- Counts ---
    if (userDb.isOpen()) {
        QSqlQuery q(userDb);

        q.exec("SELECT COUNT(*) FROM log_food");
        if (q.next()) lblFoodLogs->setText(QString::number(q.value(0).toInt()));

        q.exec("SELECT COUNT(*) FROM custom_food");
        if (q.next()) lblCustomFoods->setText(QString::number(q.value(0).toInt()));

        q.exec("SELECT COUNT(*) FROM rda");
        if (q.next()) lblRdaOverrides->setText(QString::number(q.value(0).toInt()));
    }

    // --- Recipe Count (from filesystem) ---
    QString recipePath = QDir::homePath() + "/.nutra/recipe";
    QDir recipeDir(recipePath);
    int recipeCount = 0;
    if (recipeDir.exists()) {
        recipeCount = recipeDir.entryList(QDir::Files).count();
    }
    lblRecipes->setText(QString::number(recipeCount));

    // --- Snapshot Count and Size ---
    QString backupPath = QDir::homePath() + "/.nutra/backups";
    QDir backupDir(backupPath);
    int snapshotCount = 0;
    qint64 totalBackupSize = 0;
    if (backupDir.exists()) {
        QFileInfoList files = backupDir.entryInfoList({"*.sql.gz"}, QDir::Files);
        snapshotCount = files.count();
        for (const auto& fi : files) {
            totalBackupSize += fi.size();
        }
    }
    lblSnapshots->setText(QString::number(snapshotCount));
    lblBackupSize->setText(formatBytes(totalBackupSize));

    // --- Database Sizes ---
    if (usdaDb.isOpen()) {
        QFileInfo usdaInfo(usdaDb.databaseName());
        lblUsdaSize->setText(formatBytes(usdaInfo.size()));
    }

    if (userDb.isOpen()) {
        QFileInfo userInfo(userDb.databaseName());
        lblUserSize->setText(formatBytes(userInfo.size()));
    }
}

QString PreferencesDialog::formatBytes(qint64 bytes) const {
    if (bytes < 1024) return QString("%1 B").arg(bytes);
    if (bytes < 1024 * 1024) return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    if (bytes < 1024 * 1024 * 1024)
        return QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 2);
    return QString("%1 GB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
}
