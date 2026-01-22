#ifndef PROFILESETTINGSWIDGET_H
#define PROFILESETTINGSWIDGET_H

#include <QDate>
#include <QWidget>

class QLineEdit;
class QDateEdit;
class QComboBox;
class QDoubleSpinBox;
class QSlider;
class QLabel;

class ProfileSettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit ProfileSettingsWidget(QWidget* parent = nullptr);

    // Save current profile data to database
    void save();

private:
    void setupUi();
    void loadProfile();
    void ensureSchema();  // Check and add columns if missing

    QLineEdit* nameEdit;
    QDateEdit* dobEdit;
    QComboBox* sexCombo;
    QDoubleSpinBox* heightSpin;
    QDoubleSpinBox* weightSpin;
    QSlider* activitySlider;
    QLabel* activityLabel;
};

#endif  // PROFILESETTINGSWIDGET_H
