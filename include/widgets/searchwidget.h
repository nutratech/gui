#ifndef SEARCHWIDGET_H
#define SEARCHWIDGET_H

#include <QCompleter>
#include <QDateTime>
#include <QLineEdit>
#include <QPushButton>
#include <QStringListModel>
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
    void onCompleterActivated(const QString& text);

private:
    void addToHistory(int foodId, const QString& foodName);
    void loadHistory();
    void updateCompleterModel();

    QLineEdit* searchInput;
    QTableWidget* resultsTable;
    FoodRepository repository;
    QTimer* searchTimer;

    QCompleter* historyCompleter;
    QStringListModel* historyModel;

    struct HistoryItem {
        int id;
        QString name;
        QDateTime timestamp;
    };
    QList<HistoryItem> recentHistory;
};

#endif  // SEARCHWIDGET_H
