#include "widgets/dailylogwidget.h"

#include <QDebug>
#include <QHeaderView>
#include <QLabel>

DailyLogWidget::DailyLogWidget(QWidget* parent) : QWidget(parent) {
    setupUi();
    refresh();
}

void DailyLogWidget::setupUi() {
    auto* layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel("Today's Food Log", this));

    logTable = new QTableWidget(this);
    logTable->setColumnCount(4);
    logTable->setHorizontalHeaderLabels({"Meal", "Food", "Amount", "Calories"});
    logTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    layout->addWidget(logTable);
}

void DailyLogWidget::refresh() {
    updateTable();
}

void DailyLogWidget::updateTable() {
    logTable->setRowCount(0);

    // Get logs for today
    auto logs = m_mealRepo.getDailyLogs(QDate::currentDate());

    for (const auto& log : logs) {
        int row = logTable->rowCount();
        logTable->insertRow(row);

        // Meal Name
        logTable->setItem(row, 0, new QTableWidgetItem(log.mealName));

        // Food Name
        logTable->setItem(row, 1, new QTableWidgetItem(log.foodName));

        // Amount
        logTable->setItem(row, 2, new QTableWidgetItem(QString::number(log.grams, 'f', 1) + " g"));

        // Calories Calculation
        // TODO: optimize by fetching in batch or using repository better?
        // For now, simple fetch is fine for valid DBs
        auto nutrients = m_foodRepo.getFoodNutrients(log.foodId);
        double kcal = 0;
        for (const auto& nut : nutrients) {
            if (nut.id == 208) {  // KCAL
                kcal = (nut.amount * log.grams) / 100.0;
                break;
            }
        }
        logTable->setItem(row, 3, new QTableWidgetItem(QString::number(kcal, 'f', 0) + " kcal"));
    }
}
