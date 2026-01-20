#ifndef MEALWIDGET_H
#define MEALWIDGET_H

#include "db/foodrepository.h"
#include <QPushButton>
#include <QTableWidget>
#include <QWidget>
#include <map>
#include <vector>

struct MealItem {
  int foodId;
  QString name;
  double grams;
  std::vector<Nutrient> nutrients_100g; // Base nutrients
};

class MealWidget : public QWidget {
  Q_OBJECT

public:
  explicit MealWidget(QWidget *parent = nullptr);

  void addFood(int foodId, const QString &foodName, double grams);

private slots:
  void clearMeal();

private:
  void updateTotals();

  QTableWidget *itemsTable;
  QTableWidget *totalsTable;
  QPushButton *clearButton;

  std::vector<MealItem> mealItems;
  FoodRepository repository;
};

#endif // MEALWIDGET_H
