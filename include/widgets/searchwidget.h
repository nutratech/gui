#ifndef SEARCHWIDGET_H
#define SEARCHWIDGET_H

#include "db/foodrepository.h"
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>
#include <QWidget>

class SearchWidget : public QWidget {
  Q_OBJECT

public:
  explicit SearchWidget(QWidget *parent = nullptr);

signals:
  void foodSelected(int foodId, const QString &foodName);

private slots:
  void performSearch();
  void onRowDoubleClicked(int row, int column);

private:
  QLineEdit *searchInput;
  QPushButton *searchButton;
  QTableWidget *resultsTable;
  FoodRepository repository;
  QTimer *searchTimer;
};

#endif // SEARCHWIDGET_H
