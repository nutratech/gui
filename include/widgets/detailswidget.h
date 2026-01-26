#ifndef DETAILSWIDGET_H
#define DETAILSWIDGET_H

#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QWidget>

#include "db/foodrepository.h"

class DetailsWidget : public QWidget {
    Q_OBJECT

public:
    explicit DetailsWidget(QWidget* parent = nullptr);

    void loadFood(int foodId, const QString& foodName);

signals:
    void addToMeal(int foodId, const QString& foodName, double grams);

private slots:
    void onAddClicked();
    void updateTable();

private:
    QLabel* nameLabel;
    QTableWidget* nutrientsTable;
    QPushButton* addButton;
    QCheckBox* scaleCheckbox;
    QSpinBox* scaleSpinBox;
    FoodRepository repository;

    int currentFoodId;
    QString currentFoodName;
};

#endif  // DETAILSWIDGET_H
