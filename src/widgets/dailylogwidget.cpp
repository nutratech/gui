#include "widgets/dailylogwidget.h"

#include <QDebug>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QProgressBar>
#include <QSplitter>

DailyLogWidget::DailyLogWidget(QWidget* parent) : QWidget(parent) {
    setupUi();
    refresh();
}

void DailyLogWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    auto* splitter = new QSplitter(Qt::Vertical, this);
    mainLayout->addWidget(splitter);

    // --- Top: Log Table ---
    auto* topWidget = new QWidget(this);
    auto* topLayout = new QVBoxLayout(topWidget);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->addWidget(new QLabel("Today's Food Log", this));

    logTable = new QTableWidget(this);
    logTable->setColumnCount(4);
    logTable->setHorizontalHeaderLabels({"Meal", "Food", "Amount", "Calories"});
    logTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    topLayout->addWidget(logTable);

    splitter->addWidget(topWidget);

    // --- Bottom: Analysis ---
    auto* bottomWidget = new QWidget(this);
    auto* bottomLayout = new QVBoxLayout(bottomWidget);
    bottomLayout->setContentsMargins(0, 0, 0, 0);

    auto* analysisBox = new QGroupBox("Analysis (Projected)", this);
    auto* analysisLayout = new QVBoxLayout(analysisBox);

    // Scale Controls
    auto* scaleLayout = new QHBoxLayout();
    scaleLayout->addWidget(new QLabel("Project to Goal:", this));

    scaleInput = new QSpinBox(this);
    scaleInput->setRange(0, 50000);
    scaleInput->setValue(2000);  // Default Goal
    scaleLayout->addWidget(scaleInput);

    scaleLayout->addWidget(new QLabel("kcal", this));
    scaleLayout->addStretch();
    analysisLayout->addLayout(scaleLayout);

    connect(scaleInput, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &DailyLogWidget::updateAnalysis);

    // Analysis Table
    analysisTable = new QTableWidget(this);
    analysisTable->setColumnCount(3);
    analysisTable->setHorizontalHeaderLabels({"Nutrient", "Progress", "Detail"});
    analysisTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    analysisTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    analysisTable->setSelectionMode(QAbstractItemView::NoSelection);
    analysisLayout->addWidget(analysisTable);

    bottomLayout->addWidget(analysisBox);
    splitter->addWidget(bottomWidget);

    // Set initial sizes
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
}

void DailyLogWidget::refresh() {
    updateTable();
    updateAnalysis();
}

void DailyLogWidget::updateAnalysis() {
    std::map<int, double> totals;  // id -> amount
    auto logs = m_mealRepo.getDailyLogs(QDate::currentDate());

    for (const auto& log : logs) {
        auto nutrients = m_foodRepo.getFoodNutrients(log.foodId);
        double scale = log.grams / 100.0;
        for (const auto& nut : nutrients) {
            totals[nut.id] += nut.amount * scale;
        }
    }

    double goalKcal = scaleInput->value();
    double currentKcal = totals[208];
    double multiplier = 1.0;
    if (currentKcal > 0 && goalKcal > 0) {
        multiplier = goalKcal / currentKcal;
    }

    analysisTable->setRowCount(0);

    // Iterate over defined RDAs from repository
    auto rdas = m_foodRepo.getNutrientRdas();
    for (const auto& [nutrId, rda] : rdas) {
        if (rda <= 0) continue;

        double val = totals[nutrId];
        double projectedVal = val * multiplier;
        double pct = (projectedVal / rda) * 100.0;
        QString unit = m_foodRepo.getNutrientUnit(nutrId);
        QString name = m_foodRepo.getNutrientName(nutrId);

        int row = analysisTable->rowCount();
        analysisTable->insertRow(row);

        // 1. Nutrient Name
        analysisTable->setItem(row, 0, new QTableWidgetItem(name));

        // 2. Progress Bar
        auto* bar = new QProgressBar();
        bar->setRange(0, 100);
        bar->setValue(std::min(static_cast<int>(pct), 100));
        bar->setTextVisible(true);
        bar->setFormat(QString("%1%").arg(pct, 0, 'f', 1));

        // Coloring logic based on CLI thresholds
        QString color = "#3498db";  // Default Blue
        if (pct < 50)
            color = "#f1c40f";  // Yellow (Under)
        else if (pct > 150)
            color = "#8e44ad";  // Purple (Over)
        else if (pct >= 100)
            color = "#2ecc71";  // Green (Good)

        bar->setStyleSheet(QString("QProgressBar::chunk { background-color: %1; }").arg(color));
        analysisTable->setCellWidget(row, 1, bar);

        // 3. Detail Text
        QString detail = QString("%1 (%2) / %3 %4")
                             .arg(val, 0, 'f', 1)
                             .arg(projectedVal, 0, 'f', 1)
                             .arg(rda, 0, 'f', 1)
                             .arg(unit);
        analysisTable->setItem(row, 2, new QTableWidgetItem(detail));
    }
}

void DailyLogWidget::updateTable() {
    logTable->setRowCount(0);

    // Get logs for today
    auto logs = m_mealRepo.getDailyLogs(QDate::currentDate());

    for (const auto& log : logs) {
        int row = logTable->rowCount();
        logTable->insertRow(row);

        // Meal Name
        logTable->setItem(row, 0, new QTableWidgetItem(log.mealName));

        // Food Name
        logTable->setItem(row, 1, new QTableWidgetItem(log.foodName));

        // Amount
        logTable->setItem(row, 2, new QTableWidgetItem(QString::number(log.grams, 'f', 1) + " g"));

        // Calories Calculation
        // TODO: optimize by fetching in batch or using repository better?
        // For now, simple fetch is fine for valid DBs
        auto nutrients = m_foodRepo.getFoodNutrients(log.foodId);
        double kcal = 0;
        for (const auto& nut : nutrients) {
            if (nut.id == 208) {  // KCAL
                kcal = (nut.amount * log.grams) / 100.0;
                break;
            }
        }
        logTable->setItem(row, 3, new QTableWidgetItem(QString::number(kcal, 'f', 0) + " kcal"));
    }
}
