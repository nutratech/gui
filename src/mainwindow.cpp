#include "mainwindow.h"

#include <QAction>
#include <QDebug>
#include <QFileDialog>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>

#include "db/databasemanager.h"
#include "widgets/rdasettingswidget.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    for (auto& recentFileAction : recentFileActions) {
        recentFileAction = new QAction(this);
        recentFileAction->setVisible(false);
        connect(recentFileAction, &QAction::triggered, this, &MainWindow::onRecentFileClick);
    }
    setupUi();
    updateRecentFileActions();
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
    QAction* rdaAction = editMenu->addAction("RDA Settings");
    connect(rdaAction, &QAction::triggered, this, [this]() {
        RDASettingsWidget dlg(repository, this);
        dlg.exec();
    });

    QAction* settingsAction = editMenu->addAction("Settings");
    connect(settingsAction, &QAction::triggered, this, &MainWindow::onSettings);

    // Help Menu
    auto* helpMenu = menuBar()->addMenu("&Help");
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
                detailsWidget->loadFood(foodId, foodName);
                tabs->setCurrentWidget(detailsWidget);
            });

    connect(searchWidget, &SearchWidget::addToMealRequested, this,
            [=](int foodId, const QString& foodName, double grams) {
                mealWidget->addFood(foodId, foodName, grams);
                tabs->setCurrentWidget(mealWidget);
            });

    // Analysis Tab
    detailsWidget = new DetailsWidget(this);
    tabs->addTab(detailsWidget, "Analyze");

    // Meal Tab
    mealWidget = new MealWidget(this);
    tabs->addTab(mealWidget, "Meal Tracker");

    // Connect Analysis -> Meal
    connect(detailsWidget, &DetailsWidget::addToMeal, this,
            [=](int foodId, const QString& foodName, double grams) {
                mealWidget->addFood(foodId, foodName, grams);
                // Optional: switch tab?
                // tabs->setCurrentWidget(mealWidget);
            });
}

void MainWindow::onOpenDatabase() {
    QString fileName =
        QFileDialog::getOpenFileName(this, "Open USDA Database", QDir::homePath() + "/.nutra",
                                     "SQLite Databases (*.sqlite3 *.db)");

    if (!fileName.isEmpty()) {
        if (DatabaseManager::instance().connect(fileName)) {
            qDebug() << "Switched to database:" << fileName;
            addToRecentFiles(fileName);
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
        if (DatabaseManager::instance().connect(fileName)) {
            qDebug() << "Switched to database (recent):" << fileName;
            addToRecentFiles(fileName);
            QMessageBox::information(this, "Database Opened",
                                     "Successfully connected to: " + fileName);
        } else {
            QMessageBox::critical(this, "Database Error", "Failed to connect to: " + fileName);
        }
    }
}

void MainWindow::updateRecentFileActions() {
    QSettings settings("NutraTech", "Nutra");
    QStringList files = settings.value("recentFiles").toStringList();

    int numRecentFiles = static_cast<int>(
        qMin(static_cast<std::size_t>(files.size()), static_cast<std::size_t>(MaxRecentFiles)));

    for (int i = 0; i < numRecentFiles; ++i) {
        QString text = QString("&%1 %2").arg(i + 1).arg(QFileInfo(files[i]).fileName());
        recentFileActions[static_cast<std::size_t>(i)]->setText(text);
        recentFileActions[static_cast<std::size_t>(i)]->setData(files[i]);
        recentFileActions[static_cast<std::size_t>(i)]->setVisible(true);
    }
    for (int i = numRecentFiles; i < MaxRecentFiles; ++i)
        recentFileActions[static_cast<std::size_t>(i)]->setVisible(false);

    recentFilesMenu->setEnabled(numRecentFiles > 0);
}

void MainWindow::addToRecentFiles(const QString& path) {
    QSettings settings("NutraTech", "Nutra");
    QStringList files = settings.value("recentFiles").toStringList();
    files.removeAll(path);
    files.prepend(path);
    while (files.size() > MaxRecentFiles) files.removeLast();

    settings.setValue("recentFiles", files);
    updateRecentFileActions();
}

void MainWindow::onSettings() {
    QMessageBox::information(this, "Settings", "Settings dialog coming soon!");
}

void MainWindow::onAbout() {
    QMessageBox::about(this, "About Nutrient Coach",
                       QString("<h3>Nutrient Coach %1</h3>"
                               "<p>A C++/Qt application for tracking nutrition.</p>"
                               "<p>Homepage: <a "
                               "href=\"https://github.com/nutratech/gui\">https://github.com/"
                               "nutratech/gui</a></p>")
                           .arg(NUTRA_VERSION_STRING));
}
