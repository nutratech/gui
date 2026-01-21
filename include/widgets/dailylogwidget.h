#ifndef DAILYLOGWIDGET_H
#define DAILYLOGWIDGET_H

#include <QDate>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>

#include "db/foodrepository.h"
#include "db/mealrepository.h"

class DailyLogWidget : public QWidget {
    Q_OBJECT

public:
    explicit DailyLogWidget(QWidget* parent = nullptr);

public slots:
    void refresh();

private:
    void setupUi();
    void updateTable();

    QTableWidget* logTable;
    MealRepository m_mealRepo;
    FoodRepository m_foodRepo;
};

#endif  // DAILYLOGWIDGET_H
