#include "mainwindow.h"
#include <QVBoxLayout>

#include <QDebug>
#include <QLabel>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) { setupUi(); }

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
  setWindowTitle("Nutrient Coach");
  setWindowIcon(QIcon(":/resources/nutrition_icon-no_bg.png"));
  resize(1000, 700);

  auto *centralWidget = new QWidget(this);
  setCentralWidget(centralWidget);

  auto *mainLayout = new QVBoxLayout(centralWidget);

  tabs = new QTabWidget(this);
  mainLayout->addWidget(tabs);

  // Search Tab
  searchWidget = new SearchWidget(this);
  tabs->addTab(searchWidget, "Search Foods");

  // Connect signal
  connect(searchWidget, &SearchWidget::foodSelected, this,
          [=](int foodId, const QString &foodName) {
            qDebug() << "Selected food:" << foodName << "(" << foodId << ")";
            detailsWidget->loadFood(foodId, foodName);
            tabs->setCurrentWidget(detailsWidget);
          });

  // Analysis Tab
  detailsWidget = new DetailsWidget(this);
  tabs->addTab(detailsWidget, "Analyze");

  // Meal Tab
  mealWidget = new MealWidget(this);
  tabs->addTab(mealWidget, "Meal Tracker");

  // Connect Analysis -> Meal
  connect(detailsWidget, &DetailsWidget::addToMeal, this,
          [=](int foodId, const QString &foodName, double grams) {
            mealWidget->addFood(foodId, foodName, grams);
            // Optional: switch tab?
            // tabs->setCurrentWidget(mealWidget);
          });
}
