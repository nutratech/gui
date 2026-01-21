#ifndef SEARCHWIDGET_H
#define SEARCHWIDGET_H

#include <QDateTime>
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
    void searchStatus(const QString& msg);

private slots:
    void performSearch();
    void onRowDoubleClicked(int row, int column);
    void onCustomContextMenu(const QPoint& pos);
    void showHistory();

private:
    void addToHistory(int foodId, const QString& foodName);
    void loadHistory();

    QLineEdit* searchInput;
    QPushButton* searchButton;
    QPushButton* historyButton;
    QTableWidget* resultsTable;
    FoodRepository repository;
    QTimer* searchTimer;

    struct HistoryItem {
        int id;
        QString name;
        QDateTime timestamp;
    };
    QList<HistoryItem> recentHistory;
};

#endif  // SEARCHWIDGET_H
