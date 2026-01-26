#include "widgets/detailswidget.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QProgressBar>
#include <QSettings>
#include <QToolButton>
#include <QVBoxLayout>

DetailsWidget::DetailsWidget(QWidget* parent) : QWidget(parent), currentFoodId(-1) {
    auto* layout = new QVBoxLayout(this);

    // Header
    auto* headerLayout = new QHBoxLayout();
    nameLabel = new QLabel("No food selected", this);
    QFont font = nameLabel->font();
    font.setPointSize(14);
    font.setBold(true);
    nameLabel->setFont(font);
    // Context Menu
    nameLabel->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(nameLabel, &QLabel::customContextMenuRequested, this, [this](const QPoint& pos) {
        if (currentFoodId == -1) return;

        QMenu menu(this);
        QAction* copyAction = menu.addAction("Copy Food ID");
        connect(copyAction, &QAction::triggered, this,
                [this]() { QApplication::clipboard()->setText(QString::number(currentFoodId)); });
        menu.exec(nameLabel->mapToGlobal(pos));
    });
    addButton = new QPushButton("Add to Meal", this);
    addButton->setEnabled(false);
    connect(addButton, &QPushButton::clicked, this, &DetailsWidget::onAddClicked);

    headerLayout->addWidget(nameLabel);

    copyIdBtn = new QToolButton(this);
    copyIdBtn->setText("Copy ID");
    copyIdBtn->setVisible(false);
    connect(copyIdBtn, &QToolButton::clicked, this, [this]() {
        if (currentFoodId != -1) {
            QApplication::clipboard()->setText(QString::number(currentFoodId));
        }
    });
    headerLayout->addWidget(copyIdBtn);

    headerLayout->addStretch();
    clearButton = new QPushButton("Clear", this);
    clearButton->setVisible(false);
    connect(clearButton, &QPushButton::clicked, this, &DetailsWidget::clear);

    headerLayout->addWidget(clearButton);
    headerLayout->addWidget(addButton);
    layout->addLayout(headerLayout);

    // Scaling Controls
    auto* scaleLayout = new QHBoxLayout();
    scaleCheckbox = new QCheckBox("Scale to:", this);
    scaleSpinBox = new QSpinBox(this);
    scaleSpinBox->setRange(500, 10000);
    scaleSpinBox->setSingleStep(50);
    scaleSpinBox->setSuffix(" kcal");

    // Load last target
    QSettings settings("nutra", "nutra");
    scaleSpinBox->setValue(settings.value("analysisTargetKcal", 2000).toInt());
    scaleCheckbox->setChecked(false);  // Default off

    scaleLayout->addStretch();

    hideEmptyCheckbox = new QCheckBox("Hide Empty", this);
    hideEmptyCheckbox->setChecked(false);  // Default show all
    connect(hideEmptyCheckbox, &QCheckBox::toggled, this, &DetailsWidget::updateTable);
    scaleLayout->addWidget(hideEmptyCheckbox);

    scaleLayout->addWidget(scaleCheckbox);
    scaleLayout->addWidget(scaleSpinBox);
    layout->addLayout(scaleLayout);

    connect(scaleCheckbox, &QCheckBox::toggled, this, &DetailsWidget::updateTable);
    connect(scaleSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val) {
        QSettings settings("nutra", "nutra");
        settings.setValue("analysisTargetKcal", val);
        if (scaleCheckbox->isChecked()) updateTable();
    });

    // Nutrients Table
    nutrientsTable = new QTableWidget(this);
    nutrientsTable->setColumnCount(3);
    nutrientsTable->setHorizontalHeaderLabels({"Nutrient", "Progress", "Detail"});
    nutrientsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    nutrientsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    nutrientsTable->setSelectionMode(QAbstractItemView::NoSelection);
    layout->addWidget(nutrientsTable);
}

void DetailsWidget::loadFood(int foodId, const QString& foodName) {
    currentFoodId = foodId;
    currentFoodName = foodName;
    nameLabel->setText(foodName + QString(" (ID: %1)").arg(foodId));
    addButton->setEnabled(true);
    copyIdBtn->setVisible(true);
    clearButton->setVisible(true);
    updateTable();
}

void DetailsWidget::clear() {
    currentFoodId = -1;
    currentFoodName.clear();
    nameLabel->setText("No food selected");
    addButton->setEnabled(false);
    copyIdBtn->setVisible(false);
    clearButton->setVisible(false);
    nutrientsTable->setRowCount(0);
}

void DetailsWidget::updateTable() {
    if (currentFoodId == -1) return;

    nutrientsTable->setRowCount(0);

    std::vector<Nutrient> nutrients = repository.getFoodNutrients(currentFoodId);
    auto rdas = repository.getNutrientRdas();

    double multiplier = calculateScaleMultiplier(nutrients);
    bool hideEmpty = hideEmptyCheckbox->isChecked();

    for (const auto& nut : nutrients) {
        double multiplierVal = nut.amount * multiplier;

        if (hideEmpty && multiplierVal < 0.01) {
            continue;
        }

        addNutrientRow(nut, multiplier, rdas);
    }
}

double DetailsWidget::calculateScaleMultiplier(const std::vector<Nutrient>& nutrients) {
    if (!scaleCheckbox->isChecked()) return 1.0;

    // Find calories (ID 208)
    double kcalPer100g = 0;
    for (const auto& n : nutrients) {
        if (n.id == 208) {
            kcalPer100g = n.amount;
            break;
        }
    }

    double target = scaleSpinBox->value();
    if (kcalPer100g > 0 && target > 0) {
        return target / kcalPer100g;
    }
    return 1.0;
}

void DetailsWidget::addNutrientRow(const Nutrient& nut, double multiplier,
                                   const std::map<int, double>& rdas) {
    int row = nutrientsTable->rowCount();
    nutrientsTable->insertRow(row);

    nutrientsTable->setItem(row, 0, new QTableWidgetItem(nut.description));

    double rda = 0;
    if (rdas.count(nut.id) != 0U) {
        rda = rdas.at(nut.id);
    }

    double val = nut.amount * multiplier;

    // Progress Bar
    auto* bar = new QProgressBar();
    bar->setRange(0, 100);
    int pct = 0;
    if (rda > 0) pct = static_cast<int>((val / rda) * 100.0);
    bar->setValue(std::min(pct, 100));
    bar->setTextVisible(true);
    bar->setFormat(QString("%1%").arg(pct));

    // Color logic
    QString color = "#bdc3c7";  // Grey if no RDA
    if (rda > 0) {
        color = "#3498db";  // Blue
        if (pct < 50)
            color = "#f1c40f";  // Yellow
        else if (pct > 150)
            color = "#8e44ad";  // Purple
        else if (pct >= 100)
            color = "#2ecc71";  // Green
    }
    bar->setStyleSheet(QString("QProgressBar::chunk { background-color: %1; }").arg(color));
    nutrientsTable->setCellWidget(row, 1, bar);

    // Detail
    QString detail;
    if (rda > 0) {
        detail = QString("%1 / %2 %3").arg(val, 0, 'f', 1).arg(rda, 0, 'f', 1).arg(nut.unit);
    } else {
        detail = QString("%1 %2").arg(val, 0, 'f', 1).arg(nut.unit);
    }
    nutrientsTable->setItem(row, 2, new QTableWidgetItem(detail));
}

void DetailsWidget::onAddClicked() {
    if (currentFoodId != -1) {
        // Default 100g
        emit addToMeal(currentFoodId, currentFoodName, 100.0);
    }
}
