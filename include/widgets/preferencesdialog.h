#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>

class QLabel;
class QTabWidget;

class PreferencesDialog : public QDialog {
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget* parent = nullptr);

private:
    void setupUi();
    void loadStatistics();
    QString formatBytes(qint64 bytes) const;

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
};

#endif  // PREFERENCESDIALOG_H
