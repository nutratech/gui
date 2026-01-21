#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "widgets/detailswidget.h"
#include "widgets/mealwidget.h"
#include "widgets/searchwidget.h"
#include <QMainWindow>
#include <QTabWidget>

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override;

private:
  void setupUi();

  QTabWidget *tabs;
  SearchWidget *searchWidget;
  DetailsWidget *detailsWidget;
  MealWidget *mealWidget;

private slots:
  void onAbout();
};

#endif // MAINWINDOW_H
