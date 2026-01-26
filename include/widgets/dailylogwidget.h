#ifndef DAILYLOGWIDGET_H
#define DAILYLOGWIDGET_H

#include <QDate>
#include <QGroupBox>
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

private:
    void setupUi();
    void updateTable();

    QTableWidget* logTable;

    // Analysis UI
    QGroupBox* analysisBox;
    QVBoxLayout* analysisLayout;
    QTableWidget* analysisTable;
    QSpinBox* scaleInput;

    MealRepository m_mealRepo;
    FoodRepository m_foodRepo;

    void updateAnalysis();
};

#endif  // DAILYLOGWIDGET_H
