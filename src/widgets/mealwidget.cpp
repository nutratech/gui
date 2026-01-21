#include "widgets/mealwidget.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

MealWidget::MealWidget(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);

    // Items List
    layout->addWidget(new QLabel("Meal Composition (Builder)", this));
    itemsTable = new QTableWidget(this);
    itemsTable->setColumnCount(3);
    itemsTable->setHorizontalHeaderLabels({"Food", "Grams", "Calories"});
    itemsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    layout->addWidget(itemsTable);

    // Controls
    auto* buttonLayout = new QHBoxLayout();

    auto* addToLogButton = new QPushButton("Add to Log", this);
    connect(addToLogButton, &QPushButton::clicked, this, &MealWidget::onAddToLog);
    buttonLayout->addWidget(addToLogButton);

    clearButton = new QPushButton("Clear Builder", this);
    connect(clearButton, &QPushButton::clicked, this, &MealWidget::clearMeal);
    buttonLayout->addWidget(clearButton);

    layout->addLayout(buttonLayout);

    // Totals
    layout->addWidget(new QLabel("Predicted Nutrition", this));
    totalsTable = new QTableWidget(this);
    totalsTable->setColumnCount(3);
    totalsTable->setHorizontalHeaderLabels({"Nutrient", "Total", "Unit"});
    totalsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    layout->addWidget(totalsTable);

    refresh();
}

void MealWidget::addFood(int foodId, const QString& foodName, double grams) {
    std::vector<Nutrient> baseNutrients = repository.getFoodNutrients(foodId);

    MealItem item;
    item.foodId = foodId;
    item.name = foodName;
    item.grams = grams;
    item.nutrients_100g = baseNutrients;
    mealItems.push_back(item);

    refresh();
}

void MealWidget::onAddToLog() {
    if (mealItems.empty()) return;

    // TODO: Add meal selection dialog? For now default to Breakfast/General (1)
    int mealId = 1;

    for (const auto& item : mealItems) {
        m_mealRepo.addFoodLog(item.foodId, item.grams, mealId);
    }

    emit logUpdated();

    mealItems.clear();
    refresh();

    QMessageBox::information(this, "Logged", "Meal added to daily log.");
}

void MealWidget::refresh() {
    itemsTable->setRowCount(0);

    for (const auto& item : mealItems) {
        int row = itemsTable->rowCount();
        itemsTable->insertRow(row);
        itemsTable->setItem(row, 0, new QTableWidgetItem(item.name));
        itemsTable->setItem(row, 1, new QTableWidgetItem(QString::number(item.grams)));

        double kcal = 0;
        for (const auto& nut : item.nutrients_100g) {
            if (nut.id == 208) {
                kcal = (nut.amount * item.grams) / 100.0;
                break;
            }
        }
        itemsTable->setItem(row, 2, new QTableWidgetItem(QString::number(kcal, 'f', 1)));
    }

    updateTotals();
}

void MealWidget::clearMeal() {
    if (mealItems.empty()) return;

    auto reply = QMessageBox::question(this, "Clear Builder",
                                       "Are you sure you want to clear the current meal builder?",
                                       QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        mealItems.clear();
        refresh();
    }
}

void MealWidget::updateTotals() {
    std::map<int, double> totals;  // id -> amount
    std::map<int, QString> units;
    std::map<int, QString> names;

    for (const auto& item : mealItems) {
        double scale = item.grams / 100.0;
        for (const auto& nut : item.nutrients_100g) {
            totals[nut.id] += nut.amount * scale;
            names.try_emplace(nut.id, nut.description);
            units.try_emplace(nut.id, nut.unit);
        }
    }

    totalsTable->setRowCount(static_cast<int>(totals.size()));
    int row = 0;
    for (const auto& pair : totals) {
        int nid = pair.first;
        double amount = pair.second;

        totalsTable->setItem(row, 0, new QTableWidgetItem(names[nid]));
        totalsTable->setItem(row, 1, new QTableWidgetItem(QString::number(amount, 'f', 2)));
        totalsTable->setItem(row, 2, new QTableWidgetItem(units[nid]));
        row++;
    }
}
