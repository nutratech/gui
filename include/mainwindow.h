#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <array>

#include "widgets/detailswidget.h"
#include "widgets/mealwidget.h"
#include "widgets/searchwidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onOpenDatabase();
    void onRecentFileClick();
    void onSettings();
    void onAbout();

private:
    void setupUi();
    void updateRecentFileActions();
    void addToRecentFiles(const QString& path);

    QTabWidget* tabs;
    SearchWidget* searchWidget;
    DetailsWidget* detailsWidget;
    MealWidget* mealWidget;
    FoodRepository repository;

    QMenu* recentFilesMenu;
    static constexpr int MaxRecentFiles = 5;
    std::array<QAction*, MaxRecentFiles> recentFileActions;
};

#endif  // MAINWINDOW_H
