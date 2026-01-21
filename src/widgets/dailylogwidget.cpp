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

    // Analysis UI
    kcalBar = nullptr;
    proteinBar = nullptr;
    carbsBar = nullptr;
    fatBar = nullptr;

    // Scale Controls
    auto* scaleLayout = new QHBoxLayout();
    scaleLayout->addWidget(new QLabel("Project to Goal:", this));

    scaleInput = new QSpinBox(this);
    scaleInput->setRange(0, 50000);
    scaleInput->setValue(2000);  // Default Goal
    scaleLayout->addWidget(scaleInput);

    scaleLayout->addWidget(new QLabel("kcal", this));

    // Add spacer
    scaleLayout->addStretch();

    analysisLayout->addLayout(scaleLayout);

    connect(scaleInput, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &DailyLogWidget::updateAnalysis);

    createProgressBar(analysisLayout, "Calories", kcalBar, "#3498db");    // Blue
    createProgressBar(analysisLayout, "Protein", proteinBar, "#e74c3c");  // Red
    createProgressBar(analysisLayout, "Carbs", carbsBar, "#f1c40f");      // Yellow
    createProgressBar(analysisLayout, "Fat", fatBar, "#2ecc71");          // Green

    bottomLayout->addWidget(analysisBox);
    splitter->addWidget(bottomWidget);

    // Set initial sizes
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
}

void DailyLogWidget::createProgressBar(QVBoxLayout* layout, const QString& label,
                                       QProgressBar*& bar, const QString& color) {
    auto* hLayout = new QHBoxLayout();
    hLayout->addWidget(new QLabel(label + ":"));

    bar = new QProgressBar();
    bar->setRange(0, 100);
    bar->setValue(0);
    bar->setTextVisible(true);
    bar->setStyleSheet(QString("QProgressBar::chunk { background-color: %1; }").arg(color));

    hLayout->addWidget(bar);
    layout->addLayout(hLayout);
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

    // Hardcoded RDAs for now (TODO: Fetch from FoodRepository/User Profile)
    double goalKcal = scaleInput->value();  // Projection Target

    // Calculate Multiplier
    double currentKcal = totals[208];
    double multiplier = 1.0;
    if (currentKcal > 0 && goalKcal > 0) {
        multiplier = goalKcal / currentKcal;
    }

    // Use scaling for "What If" visualization?
    // Actually, progress bars usually show % of RDA.
    // If we project, we want to show: "If I ate this ratio until I hit 2000kcal, I would have X
    // protein."

    double rdaKcal = goalKcal;  // The goal IS the RDA in this context usually
    double rdaProtein = 150;
    double rdaCarbs = 300;
    double rdaFat = 80;

    auto updateBar = [&](QProgressBar* bar, int nutrId, double rda) {
        double val = totals[nutrId];
        double projectedVal = val * multiplier;

        int pct = 0;
        if (rda > 0) pct = static_cast<int>((projectedVal / rda) * 100.0);

        bar->setValue(std::min(pct, 100));

        // Format: "Actual (Projected) / Target"
        QString text =
            QString("%1 (%2) / %3 g").arg(val, 0, 'f', 0).arg(projectedVal, 0, 'f', 0).arg(rda);
        if (nutrId == 208)
            text = QString("%1 (%2) / %3 kcal")
                       .arg(val, 0, 'f', 0)
                       .arg(projectedVal, 0, 'f', 0)
                       .arg(rda);

        bar->setFormat(text);

        if (pct > 100) {
            bar->setStyleSheet("QProgressBar::chunk { background-color: #8e44ad; }");
        } else {
            // Reset style (hacky, ideally use separate stylesheet)
            // bar->setStyleSheet("");
        }
    };

    updateBar(kcalBar, 208, rdaKcal);
    updateBar(proteinBar, 203, rdaProtein);
    updateBar(carbsBar, 205, rdaCarbs);
    updateBar(fatBar, 204, rdaFat);
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
