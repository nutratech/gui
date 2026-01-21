#ifndef MEALWIDGET_H
#define MEALWIDGET_H

#include <QPushButton>
#include <QTableWidget>
#include <QWidget>
#include <vector>

#include "db/foodrepository.h"
#include "db/mealrepository.h"

struct MealItem {
    int foodId;
    QString name;
    double grams;
    std::vector<Nutrient> nutrients_100g;
};

class MealWidget : public QWidget {
    Q_OBJECT

public:
    explicit MealWidget(QWidget* parent = nullptr);

    void addFood(int foodId, const QString& foodName, double grams);

signals:
    void logUpdated();

private slots:
    void clearMeal();
    void onAddToLog();

private:
    void updateTotals();
    void refresh();

    QTableWidget* itemsTable;
    QPushButton* clearButton;
    QTableWidget* totalsTable;

    FoodRepository repository;
    MealRepository m_mealRepo;

    std::vector<MealItem> mealItems;
};

#endif  // MEALWIDGET_H
