#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>

#include "db/foodrepository.h"

class QLabel;
class QTabWidget;
class RDASettingsWidget;

class PreferencesDialog : public QDialog {
    Q_OBJECT

public:
    explicit PreferencesDialog(FoodRepository& repository, QWidget* parent = nullptr);

private:
    void setupUi();
    void loadStatistics();
    [[nodiscard]] QString formatBytes(qint64 bytes) const;

    QTabWidget* tabWidget;

    // Stats labels
    QLabel* lblFoodLogs;
    QLabel* lblCustomFoods;
    QLabel* lblRdaOverrides;
    QLabel* lblRecipes;
    QLabel* lblSnapshots;

    // Size labels
    QLabel* lblUsdaSize;
    QLabel* lblUserSize;
    QLabel* lblBackupSize;

    FoodRepository& m_repository;
};

#endif  // PREFERENCESDIALOG_H
