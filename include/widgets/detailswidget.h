#ifndef DETAILSWIDGET_H
#define DETAILSWIDGET_H

#include "db/foodrepository.h"
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QWidget>

class DetailsWidget : public QWidget {
  Q_OBJECT

public:
  explicit DetailsWidget(QWidget *parent = nullptr);

  void loadFood(int foodId, const QString &foodName);

signals:
  void addToMeal(int foodId, const QString &foodName, double grams);

private slots:
  void onAddClicked();

private:
  QLabel *nameLabel;
  QTableWidget *nutrientsTable;
  QPushButton *addButton;
  FoodRepository repository;

  int currentFoodId;
  QString currentFoodName;
};

#endif // DETAILSWIDGET_H
