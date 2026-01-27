#include "widgets/profilesettingswidget.h"

#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QDebug>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSlider>
#include <QSqlError>
#include <QSqlQuery>
#include <QVBoxLayout>

#include "db/databasemanager.h"

ProfileSettingsWidget::ProfileSettingsWidget(QWidget* parent) : QWidget(parent) {
    setupUi();
    ensureSchema();
    loadProfile();
}

void ProfileSettingsWidget::setupUi() {
    auto* layout = new QVBoxLayout(this);

    auto* formLayout = new QFormLayout();
    layout->addLayout(formLayout);

    // Name
    nameEdit = new QLineEdit(this);
    formLayout->addRow("Name:", nameEdit);

    // DOB
    dobEdit = new QDateEdit(this);
    dobEdit->setCalendarPopup(true);
    dobEdit->setDisplayFormat("yyyy-MM-dd");
    formLayout->addRow("Birth Date:", dobEdit);

    // Sex
    sexCombo = new QComboBox(this);
    sexCombo->addItems({"Male", "Female"});
    formLayout->addRow("Sex:", sexCombo);

    // Height
    heightSpin = new QDoubleSpinBox(this);
    heightSpin->setRange(0, 300);  // cm
    heightSpin->setSuffix(" cm");
    formLayout->addRow("Height:", heightSpin);

    // Weight
    weightSpin = new QDoubleSpinBox(this);
    weightSpin->setRange(0, 500);  // kg
    weightSpin->setSuffix(" kg");
    formLayout->addRow("Weight:", weightSpin);

    // Activity Level
    activitySlider = new QSlider(Qt::Horizontal, this);
    activitySlider->setRange(1, 5);
    activitySlider->setValue(2);  // Default to Lightly Active
    activitySlider->setTickPosition(QSlider::TicksBelow);
    activitySlider->setTickInterval(1);

    activityLabel = new QLabel("2 (Lightly Active)", this);

    auto* activityLayout = new QHBoxLayout();
    activityLayout->addWidget(activitySlider);
    activityLayout->addWidget(activityLabel);

    formLayout->addRow("Activity Level:", activityLayout);

    connect(activitySlider, &QSlider::valueChanged, this, [=](int val) {
        QString text;
        switch (val) {
            case 1:
                text = "1 (Sedentary)";
                break;
            case 2:
                text = "2 (Lightly Active)";
                break;
            case 3:
                text = "3 (Moderately Active)";
                break;
            case 4:
                text = "4 (Very Active)";
                break;
            case 5:
                text = "5 (Extra Active)";
                break;
            default:
                text = QString::number(val);
                break;
        }
        activityLabel->setText(text);
    });

    layout->addStretch();
}

void ProfileSettingsWidget::ensureSchema() {
    QSqlDatabase db = DatabaseManager::instance().userDatabase();
    if (!db.isOpen()) return;

    // Check for height column
    // SQLite doesn't support IF NOT EXISTS in ADD COLUMN well in older versions,
    // but duplicate adding errors out harmlessly usually, or we can check PRAGMA table_info.
    // We'll check PRAGMA.

    bool hasHeight = false;
    bool hasWeight = false;

    QSqlQuery q("PRAGMA table_info(profile)", db);
    while (q.next()) {
        QString col = q.value(1).toString();
        if (col == "height") hasHeight = true;
        if (col == "weight") hasWeight = true;
    }

    QSqlQuery alter(db);
    if (!hasHeight) {
        if (!alter.exec("ALTER TABLE profile ADD COLUMN height REAL")) {
            qWarning() << "Failed to add height column:" << alter.lastError().text();
        }
    }
    if (!hasWeight) {
        if (!alter.exec("ALTER TABLE profile ADD COLUMN weight REAL")) {
            qWarning() << "Failed to add weight column:" << alter.lastError().text();
        }
    }
}

void ProfileSettingsWidget::loadProfile() {
    QSqlDatabase db = DatabaseManager::instance().userDatabase();
    if (!db.isOpen()) return;

    QSqlQuery q("SELECT name, dob, gender, weight, height, act_lvl FROM profile WHERE id=1", db);
    if (q.next()) {
        nameEdit->setText(q.value(0).toString());
        dobEdit->setDate(q.value(1).toDate());

        QString sex = q.value(2).toString();
        sexCombo->setCurrentText(sex.isEmpty() ? "Male" : sex);

        weightSpin->setValue(q.value(3).toDouble());
        heightSpin->setValue(q.value(4).toDouble());

        int act = q.value(5).toInt();
        act = std::max(act, 1);
        act = std::min(act, 5);
        activitySlider->setValue(act);
    } else {
        // Default insert if missing?
        // Or assume ID 1 exists (created by init.sql?).
        // If not exists, maybe form is empty.
    }
}

void ProfileSettingsWidget::save() {
    QSqlDatabase db = DatabaseManager::instance().userDatabase();
    if (!db.isOpen()) return;

    // Check if ID 1 exists
    QSqlQuery check("SELECT 1 FROM profile WHERE id=1", db);
    bool exists = check.next();

    QSqlQuery q(db);
    if (exists) {
        q.prepare(
            "UPDATE profile SET name=?, dob=?, gender=?, weight=?, height=?, act_lvl=? WHERE id=1");
    } else {
        q.prepare(
            "INSERT INTO profile (name, dob, gender, weight, height, act_lvl, id) VALUES (?, ?, ?, "
            "?, ?, ?, 1)");
    }

    q.addBindValue(nameEdit->text());
    q.addBindValue(dobEdit->date());
    q.addBindValue(sexCombo->currentText());
    q.addBindValue(weightSpin->value());
    q.addBindValue(heightSpin->value());
    q.addBindValue(activitySlider->value());

    if (!q.exec()) {
        qCritical() << "Failed to save profile:" << q.lastError().text();
    }
}
