#ifndef SEARCHWIDGET_H
#define SEARCHWIDGET_H

#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>
#include <QWidget>

#include "db/foodrepository.h"

class SearchWidget : public QWidget {
    Q_OBJECT

public:
    explicit SearchWidget(QWidget* parent = nullptr);

signals:
    void foodSelected(int foodId, const QString& foodName);
    void addToMealRequested(int foodId, const QString& foodName, double grams);

private slots:
    void performSearch();
    void onRowDoubleClicked(int row, int column);
    void onCustomContextMenu(const QPoint& pos);

private:
    QLineEdit* searchInput;
    QPushButton* searchButton;
    QTableWidget* resultsTable;
    FoodRepository repository;
    QTimer* searchTimer;
};

#endif  // SEARCHWIDGET_H
