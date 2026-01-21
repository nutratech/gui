#ifndef RDASETTINGSWIDGET_H
#define RDASETTINGSWIDGET_H

#include <QDialog>
#include <QTableWidget>

#include "db/foodrepository.h"

class RDASettingsWidget : public QDialog {
    Q_OBJECT

public:
    explicit RDASettingsWidget(FoodRepository& repository, QWidget* parent = nullptr);

private slots:
    void onCellChanged(int row, int column);

private:
    void loadData();

    FoodRepository& m_repository;
    QTableWidget* m_table;
    bool m_loading = false;
};

#endif  // RDASETTINGSWIDGET_H
