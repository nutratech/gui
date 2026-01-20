#include "widgets/mealwidget.h"
#include <QDebug>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QVBoxLayout>

MealWidget::MealWidget(QWidget *parent) : QWidget(parent) {
  auto *layout = new QVBoxLayout(this);

  // Items List
  layout->addWidget(new QLabel("Meal Composition", this));
  itemsTable = new QTableWidget(this);
  itemsTable->setColumnCount(3);
  itemsTable->setHorizontalHeaderLabels({"Food", "Grams", "Calories"});
  itemsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
  layout->addWidget(itemsTable);

  // Controls
  clearButton = new QPushButton("Clear Meal", this);
  connect(clearButton, &QPushButton::clicked, this, &MealWidget::clearMeal);
  layout->addWidget(clearButton);

  // Totals
  layout->addWidget(new QLabel("Total Nutrition", this));
  totalsTable = new QTableWidget(this);
  totalsTable->setColumnCount(3);
  totalsTable->setHorizontalHeaderLabels({"Nutrient", "Total", "Unit"});
  totalsTable->horizontalHeader()->setSectionResizeMode(0,
                                                        QHeaderView::Stretch);
  layout->addWidget(totalsTable);
}

void MealWidget::addFood(int foodId, const QString &foodName, double grams) {
  std::vector<Nutrient> baseNutrients = repository.getFoodNutrients(foodId);

  MealItem item;
  item.foodId = foodId;
  item.name = foodName;
  item.grams = grams;
  item.nutrients_100g = baseNutrients;

  mealItems.push_back(item);

  // Update Items Table
  int row = itemsTable->rowCount();
  itemsTable->insertRow(row);
  itemsTable->setItem(row, 0, new QTableWidgetItem(foodName));
  itemsTable->setItem(row, 1, new QTableWidgetItem(QString::number(grams)));

  // Calculate Calories (ID 208 usually, or find by name?)
  // repository returns IDs based on DB. 208 is KCAL in SR28.
  double kcal = 0;
  for (const auto &nut : baseNutrients) {
    if (nut.id == 208) {
      kcal = (nut.amount * grams) / 100.0;
      break;
    }
  }
  itemsTable->setItem(row, 2,
                      new QTableWidgetItem(QString::number(kcal, 'f', 1)));

  updateTotals();
}

void MealWidget::clearMeal() {
  mealItems.clear();
  itemsTable->setRowCount(0);
  updateTotals();
}

void MealWidget::updateTotals() {
  std::map<int, double> totals; // id -> amount
  std::map<int, QString> units;
  std::map<int, QString> names;

  for (const auto &item : mealItems) {
    double scale = item.grams / 100.0;
    for (const auto &nut : item.nutrients_100g) {
      totals[nut.id] += nut.amount * scale;
      names.try_emplace(nut.id, nut.description);
      units.try_emplace(nut.id, nut.unit);
    }
  }

  totalsTable->setRowCount(static_cast<int>(totals.size()));
  int row = 0;
  for (const auto &pair : totals) {
    int nid = pair.first;
    double amount = pair.second;

    totalsTable->setItem(row, 0, new QTableWidgetItem(names[nid]));
    totalsTable->setItem(row, 1,
                         new QTableWidgetItem(QString::number(amount, 'f', 2)));
    totalsTable->setItem(row, 2, new QTableWidgetItem(units[nid]));
    row++;
  }
}
