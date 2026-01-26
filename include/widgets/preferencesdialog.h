#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>

#include "db/foodrepository.h"

class QLabel;
class QTabWidget;
class RDASettingsWidget;
class ProfileSettingsWidget;
class QSpinBox;

class PreferencesDialog : public QDialog {
    Q_OBJECT

public:
    explicit PreferencesDialog(FoodRepository& repository, QWidget* parent = nullptr);

public slots:
    void save();

private:
    void setupUi();
    void loadStatistics();
    void loadGeneralSettings();
    [[nodiscard]] QString formatBytes(qint64 bytes) const;

    QTabWidget* tabWidget;

    // General Settings
    QSpinBox* debounceSpin;
    class QCheckBox* nlpCheckBox;

    // Widgets
    ProfileSettingsWidget* profileWidget;
    RDASettingsWidget* rdaWidget;

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
