#ifndef DAILYLOGWIDGET_H
#define DAILYLOGWIDGET_H

#include <QDate>
#include <QGroupBox>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
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
    void prevDay();
    void nextDay();
    void setToday();
    void onDateChanged();

private:
    void setupUi();
    void updateTable();

    QTableWidget* logTable;

    // Analysis UI
    QGroupBox* analysisBox;
    QVBoxLayout* analysisLayout;
    QTableWidget* analysisTable;
    QSpinBox* scaleInput;

    // Date Nav
    QDate currentDate;
    QLabel* dateLabel;

    MealRepository m_mealRepo;
    FoodRepository m_foodRepo;

    void updateAnalysis();
};

#endif  // DAILYLOGWIDGET_H
