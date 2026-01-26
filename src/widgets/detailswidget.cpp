#include "widgets/detailswidget.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QProgressBar>
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

    addButton = new QPushButton("Add to Meal", this);
    addButton->setEnabled(false);
    connect(addButton, &QPushButton::clicked, this, &DetailsWidget::onAddClicked);

    headerLayout->addWidget(nameLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(addButton);
    layout->addLayout(headerLayout);

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

    nutrientsTable->setRowCount(0);

    std::vector<Nutrient> nutrients = repository.getFoodNutrients(foodId);
    auto rdas = repository.getNutrientRdas();

    // Mapping for easy lookup if needed, but vector iteration is fine
    // We want to show ALL nutrients returned for the food? Or all nutrients tracked by RDA?
    // The previous implementation showed all nutrients returned by getFoodNutrients.
    // Let's stick to that, but enhance the ones that have RDAs.

    nutrientsTable->setRowCount(static_cast<int>(nutrients.size()));
    for (int i = 0; i < static_cast<int>(nutrients.size()); ++i) {
        const auto& nut = nutrients[i];
        nutrientsTable->setItem(i, 0, new QTableWidgetItem(nut.description));

        double rda = 0;
        if (rdas.count(nut.id) != 0U) {
            rda = rdas[nut.id];
        }

        // Progress Bar
        auto* bar = new QProgressBar();
        bar->setRange(0, 100);
        int pct = 0;
        if (rda > 0) pct = static_cast<int>((nut.amount / rda) * 100.0);
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
        nutrientsTable->setCellWidget(i, 1, bar);

        // Detail
        QString detail;
        if (rda > 0) {
            detail =
                QString("%1 / %2 %3").arg(nut.amount, 0, 'f', 1).arg(rda, 0, 'f', 1).arg(nut.unit);
        } else {
            detail = QString("%1 %2").arg(nut.amount, 0, 'f', 1).arg(nut.unit);
        }
        nutrientsTable->setItem(i, 2, new QTableWidgetItem(detail));
    }
}

void DetailsWidget::onAddClicked() {
    if (currentFoodId != -1) {
        // Default 100g
        emit addToMeal(currentFoodId, currentFoodName, 100.0);
    }
}
