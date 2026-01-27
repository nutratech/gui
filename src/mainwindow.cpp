#include "mainwindow.h"

#include <QAction>
#include <QDebug>
#include <QFileDialog>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>

#include "db/databasemanager.h"
#include "widgets/preferencesdialog.h"
#include "widgets/rdasettingswidget.h"
#include "widgets/recipewidget.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    for (auto& recentFileAction : recentFileActions) {
        recentFileAction = new QAction(this);
        recentFileAction->setVisible(false);
        connect(recentFileAction, &QAction::triggered, this, &MainWindow::onRecentFileClick);
    }
    setupUi();
    updateRecentFileActions();

    // Load CSV Recipes on startup (run once)
    QSettings settings("nutra", "nutra");
    if (!settings.value("recipesLoaded", false).toBool()) {
        QString recipesPath = QDir::homePath() + "/.nutra/recipes";
        if (QDir(recipesPath).exists()) {
            RecipeRepository repo;
            repo.loadCsvRecipes(recipesPath);
            settings.setValue("recipesLoaded", true);
        }
    }
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
    setWindowTitle("Nutrient Coach");
    setWindowIcon(QIcon(":/resources/nutrition_icon-no_bg.png"));
    resize(1000, 700);

    // File Menu
    auto* fileMenu = menuBar()->addMenu("&File");
    auto* openDbAction = fileMenu->addAction("&Open Database...");
    recentFilesMenu = fileMenu->addMenu("Recent Databases");
    fileMenu->addSeparator();
    auto* exitAction = fileMenu->addAction("E&xit");
    connect(openDbAction, &QAction::triggered, this, &MainWindow::onOpenDatabase);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    for (auto& recentFileAction : recentFileActions) recentFilesMenu->addAction(recentFileAction);

    // Edit Menu
    QMenu* editMenu = menuBar()->addMenu("Edit");

    QAction* preferencesAction = editMenu->addAction("Preferences");
    connect(preferencesAction, &QAction::triggered, this, [this]() {
        PreferencesDialog dlg(repository, this);
        if (dlg.exec() == QDialog::Accepted) {
            searchWidget->reloadSettings();
        }
    });

    // Help Menu
    auto* helpMenu = menuBar()->addMenu("&Help");

    auto* licenseAction = helpMenu->addAction("&License");
    connect(licenseAction, &QAction::triggered, this, [this]() {
        QMessageBox::information(
            this, "License",
            "<h3>GNU General Public License v3.0</h3>"
            "<p>This program is free software: you can redistribute it and/or modify "
            "it under the terms of the GNU General Public License as published by "
            "the Free Software Foundation, either version 3 of the License, or "
            "(at your option) any later version.</p>"
            "<p>This program is distributed in the hope that it will be useful, "
            "but WITHOUT ANY WARRANTY; without even the implied warranty of "
            "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.</p>"
            "<p>See <a href=\"https://www.gnu.org/licenses/gpl-3.0.html\">"
            "https://www.gnu.org/licenses/gpl-3.0.html</a> for details.</p>");
    });

    auto* aboutAction = helpMenu->addAction("&About");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);

    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto* mainLayout = new QVBoxLayout(centralWidget);

    tabs = new QTabWidget(this);
    mainLayout->addWidget(tabs);

    // Search Tab
    searchWidget = new SearchWidget(this);
    tabs->addTab(searchWidget, "Search Foods");

    // Connect signal
    connect(searchWidget, &SearchWidget::foodSelected, this,
            [=](int foodId, const QString& foodName) {
                qDebug() << "Selected food:" << foodName << "(" << foodId << ")";
                // Determine if we are in analysis mode or just searching
                // For now, simpler handling:
                detailsWidget->loadFood(foodId, foodName);
                tabs->setCurrentWidget(detailsWidget);

                // Persist selection
                QSettings settings("nutra", "nutra");
                settings.setValue("lastSelectedFoodId", foodId);
                settings.setValue("lastSelectedFoodName", foodName);
            });

    connect(searchWidget, &SearchWidget::addToMealRequested, this,
            [=](int foodId, const QString& foodName, double grams) {
                mealWidget->addFood(foodId, foodName, grams);
                tabs->setCurrentWidget(mealWidget);
            });

    connect(searchWidget, &SearchWidget::searchStatus, this,
            [=](const QString& msg) { dbStatusLabel->setText(msg); });

    // Analysis Tab
    detailsWidget = new DetailsWidget(this);
    tabs->addTab(detailsWidget, "Analyze");

    // Meal Tab (Builder)
    mealWidget = new MealWidget(this);
    tabs->addTab(mealWidget, "Meal Builder");

    // Recipes Tab
    recipeWidget = new RecipeWidget(this);
    tabs->addTab(recipeWidget, "Recipes");

    // Daily Log Tab
    dailyLogWidget = new DailyLogWidget(this);
    tabs->addTab(dailyLogWidget, "Daily Log");

    // Connect Analysis -> Meal
    connect(detailsWidget, &DetailsWidget::addToMeal, this,
            [=](int foodId, const QString& foodName, double grams) {
                mealWidget->addFood(foodId, foodName, grams);
            });

    // Connect Meal Builder -> Daily Log
    connect(mealWidget, &MealWidget::logUpdated, dailyLogWidget, &DailyLogWidget::refresh);

    // Status Bar
    dbStatusLabel = new QLabel(this);
    dbStatusLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    statusBar()->addPermanentWidget(dbStatusLabel);
    updateStatusBar();

    // Restore last selection if available
    QSettings settings("nutra", "nutra");
    if (settings.contains("lastSelectedFoodId") && settings.contains("lastSelectedFoodName")) {
        int id = settings.value("lastSelectedFoodId").toInt();
        QString name = settings.value("lastSelectedFoodName").toString();
        // Defer slightly to ensure DB is ready if needed (though it should be open by now)
        QTimer::singleShot(200, this, [this, id, name]() {
            detailsWidget->loadFood(id, name);
            // Optionally switch tab? Default is usually search or dashboard.
            // Let's keep the user's focus where it makes sense.
            // If they had a selection open, maybe they want to see it.
            // tabs->setCurrentWidget(detailsWidget);
        });
    }
}

void MainWindow::updateStatusBar() {
    auto& dbMgr = DatabaseManager::instance();
    QStringList parts;
    QString tooltip = "Active Databases:\n";

    if (dbMgr.isOpen()) {
        parts << "USDA [Connected]";
        tooltip += QString("- USDA: %1\n").arg(dbMgr.database().databaseName());
    } else {
        parts << "USDA [Disconnected]";
    }

    if (dbMgr.userDatabase().isOpen()) {
        parts << "NTDB (User) [Connected]";
        tooltip += QString("- NTDB: %1\n").arg(dbMgr.userDatabase().databaseName());
    } else {
        parts << "NTDB (User) [Disconnected]";
    }

    dbStatusLabel->setText("DB Status: " + parts.join(" | "));
    dbStatusLabel->setToolTip(tooltip.trimmed());
}

void MainWindow::onOpenDatabase() {
    QString fileName =
        QFileDialog::getOpenFileName(this, "Open USDA Database", QDir::homePath() + "/.nutra",
                                     "SQLite Databases (*.sqlite3 *.db)");

    if (!fileName.isEmpty()) {
        if (DatabaseManager::instance().isOpen() &&
            DatabaseManager::instance().database().databaseName() == fileName) {
            QMessageBox::information(this, "Already Open", "This database is already loaded.");
            return;
        }

        if (DatabaseManager::instance().connect(fileName)) {
            qDebug() << "Switched to database:" << fileName;
            addToRecentFiles(fileName);
            updateStatusBar();
            // In a real app, we'd emit a signal or refresh all widgets
            // For now, let's just log and show success
            QMessageBox::information(this, "Database Opened",
                                     "Successfully connected to: " + fileName);
        } else {
            QMessageBox::critical(this, "Database Error", "Failed to connect to the database.");
        }
    }
}

void MainWindow::onRecentFileClick() {
    auto* action = qobject_cast<QAction*>(sender());
    if (action != nullptr) {
        QString fileName = action->data().toString();

        if (DatabaseManager::instance().isOpen() &&
            DatabaseManager::instance().database().databaseName() == fileName) {
            QMessageBox::information(this, "Already Open", "This database is already loaded.");
            return;
        }

        if (DatabaseManager::instance().connect(fileName)) {
            qDebug() << "Switched to database (recent):" << fileName;
            addToRecentFiles(fileName);
            updateStatusBar();
            QMessageBox::information(this, "Database Opened",
                                     "Successfully connected to: " + fileName);
        } else {
            QMessageBox::critical(this, "Database Error", "Failed to connect to: " + fileName);
        }
    }
}

void MainWindow::updateRecentFileActions() {
    QSettings settings("nutra", "nutra");

    // Check for legacy setting if new one is empty
    if (!settings.contains("recentFilesList") && settings.contains("recentFiles")) {
        QStringList legacyFiles = settings.value("recentFiles").toStringList();
        QList<QVariant> newFiles;
        for (const auto& path : legacyFiles) {
            auto info = DatabaseManager::instance().getDatabaseInfo(path);
            if (info.isValid) {  // Only migrate valid ones
                QVariantMap entry;
                entry["path"] = path;
                entry["type"] = info.type;
                entry["version"] = info.version;
                newFiles.append(entry);
            }
        }
        settings.setValue("recentFilesList", newFiles);
        settings.remove("recentFiles");  // Clean up legacy
    }

    QList<QVariant> files = settings.value("recentFilesList").toList();

    // Sort: User first, then USDA. Within type, preserve order (recency) or sort by name?
    // Usually "Recent" implies recency. But user asked for "User on top".
    // So we split into two lists (preserving recency within them) and concat.

    QList<QVariantMap> userDBs;
    QList<QVariantMap> usdaDBs;

    for (const auto& v : files) {
        QVariantMap m = v.toMap();
        if (m["type"].toString() == "User") {
            userDBs.append(m);
        } else {
            usdaDBs.append(m);
        }
    }

    QList<QVariantMap> sortedFiles = userDBs;
    sortedFiles.append(usdaDBs);

    int numToShow = static_cast<int>(qMin(static_cast<std::size_t>(sortedFiles.size()),
                                          static_cast<std::size_t>(MaxRecentFiles)));

    QFontMetrics fontMetrics(recentFilesMenu->font());

    for (int i = 0; i < numToShow; ++i) {
        QVariantMap m = sortedFiles[i];
        QString path = m["path"].toString();
        QString type = m["type"].toString();
        int version = m["version"].toInt();

        // Elide path to ~400 pixels (roughly 50-60 chars depending on font)
        QString elidedPath = fontMetrics.elidedText(path, Qt::ElideMiddle, 400);

        // Format: "/path/to/file... (User v9)"
        QString text = QString("&%1 %2 (%3 v%4)").arg(i + 1).arg(elidedPath).arg(type).arg(version);

        recentFileActions[static_cast<std::size_t>(i)]->setText(text);
        recentFileActions[static_cast<std::size_t>(i)]->setData(path);
        recentFileActions[static_cast<std::size_t>(i)]->setVisible(true);
    }
    for (int i = numToShow; i < MaxRecentFiles; ++i)
        recentFileActions[static_cast<std::size_t>(i)]->setVisible(false);

    recentFilesMenu->setEnabled(numToShow > 0);
}

void MainWindow::addToRecentFiles(const QString& path) {
    if (path.isEmpty()) return;

    auto info = DatabaseManager::instance().getDatabaseInfo(path);
    if (!info.isValid) return;

    QSettings settings("nutra", "nutra");
    // Read list of QVariantMaps
    QList<QVariant> files = settings.value("recentFilesList").toList();

    // Remove existing entry for this path
    for (int i = 0; i < files.size(); ++i) {
        if (files[i].toMap()["path"].toString() == path) {
            files.removeAt(i);
            break;
        }
    }

    // Prepare new entry
    QVariantMap entry;
    entry["path"] = path;
    entry["type"] = info.type;
    entry["version"] = info.version;

    // Prepend new entry
    files.prepend(entry);

    // Limit list size
    while (files.size() > MaxRecentFiles) {
        files.removeLast();
    }

    settings.setValue("recentFilesList", files);
    updateRecentFileActions();
}

void MainWindow::onSettings() {
    QMessageBox::information(this, "Settings", "Settings dialog coming soon!");
}

void MainWindow::onAbout() {
    QMessageBox::about(this, "About Nutrient Coach",
                       QString("<h3>Nutrient Coach %1</h3>"
                               "<p>This application is a tool designed not as a weight-loss app "
                               "but as a true <b>nutrition coach</b>, giving insights into what "
                               "you're getting and what you're lacking, empowering you to make "
                               "more informed and healthy decisions and live more of the vibrant "
                               "life you were put here for.</p>"
                               "<p>Homepage: <a "
                               "href=\"https://github.com/nutratech/gui\">https://github.com/"
                               "nutratech/gui</a></p>")
                           .arg(NUTRA_VERSION_STRING));
}
