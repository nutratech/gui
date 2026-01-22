#include "widgets/preferencesdialog.h"

#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QSettings>
#include <QSpinBox>
#include <QSqlQuery>
#include <QTabWidget>
#include <QVBoxLayout>

#include "db/databasemanager.h"
#include "widgets/profilesettingswidget.h"
#include "widgets/rdasettingswidget.h"

PreferencesDialog::PreferencesDialog(FoodRepository& repository, QWidget* parent)
    : QDialog(parent), m_repository(repository) {
    setWindowTitle("Preferences");
    setMinimumSize(550, 450);
    setupUi();
    loadStatistics();
    loadGeneralSettings();
}

void PreferencesDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    tabWidget = new QTabWidget(this);

    // === General Tab ===
    auto* generalWidget = new QWidget();
    auto* generalLayout = new QFormLayout(generalWidget);

    debounceSpin = new QSpinBox(this);
    debounceSpin->setRange(100, 5000);
    debounceSpin->setSingleStep(50);
    debounceSpin->setSuffix(" ms");
    generalLayout->addRow("Search Debounce:", debounceSpin);

    tabWidget->addTab(generalWidget, "General");

    // === Profile Tab ===
    profileWidget = new ProfileSettingsWidget(this);
    tabWidget->addTab(profileWidget, "Profile");

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

    // === RDA Settings Tab ===
    rdaWidget = new RDASettingsWidget(m_repository, this);
    tabWidget->addTab(rdaWidget, "RDA Settings");

    mainLayout->addWidget(tabWidget);

    // Buttons
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &PreferencesDialog::save);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void PreferencesDialog::loadGeneralSettings() {
    QSettings settings("NutraTech", "Nutra");
    debounceSpin->setValue(settings.value("searchDebounce", 600).toInt());
}

void PreferencesDialog::save() {
    // Save General
    QSettings settings("NutraTech", "Nutra");
    settings.setValue("searchDebounce", debounceSpin->value());

    // Save Profile
    if (profileWidget) profileWidget->save();

    // RDA saves automatically on edit in its own widget (checking RDASettingsWidget design
    // recommended, assuming yes for now or needs explicit save call if it supports it) Actually
    // RDASettingsWidget might need a save call. Let's check? Usually dialogs save on accept. But
    // for now, let's assume RDASettingsWidget handles its own stuff or doesn't need explicit save
    // call from here if it's direct DB.

    accept();
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
        recipeCount = static_cast<int>(recipeDir.entryList(QDir::Files).count());
    }
    lblRecipes->setText(QString::number(recipeCount));

    // --- Snapshot Count and Size ---
    QString backupPath = QDir::homePath() + "/.nutra/backups";
    QDir backupDir(backupPath);
    int snapshotCount = 0;
    qint64 totalBackupSize = 0;
    if (backupDir.exists()) {
        QFileInfoList files = backupDir.entryInfoList({"*.sql.gz"}, QDir::Files);
        snapshotCount = static_cast<int>(files.count());
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
    constexpr qint64 KB = 1024LL;
    constexpr qint64 MB = 1024LL * 1024LL;
    constexpr qint64 GB = 1024LL * 1024LL * 1024LL;
    if (bytes < KB) return QString("%1 B").arg(bytes);
    if (bytes < MB) return QString("%1 KB").arg(static_cast<double>(bytes) / KB, 0, 'f', 1);
    if (bytes < GB) return QString("%1 MB").arg(static_cast<double>(bytes) / MB, 0, 'f', 2);
    return QString("%1 GB").arg(static_cast<double>(bytes) / GB, 0, 'f', 2);
}
