#include "widgets/detailswidget.h"
#include <QDebug>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QVBoxLayout>

DetailsWidget::DetailsWidget(QWidget *parent)
    : QWidget(parent), currentFoodId(-1) {
  auto *layout = new QVBoxLayout(this);

  // Header
  auto *headerLayout = new QHBoxLayout();
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
  nutrientsTable->setHorizontalHeaderLabels({"Nutrient", "Amount", "Unit"});
  nutrientsTable->horizontalHeader()->setSectionResizeMode(
      0, QHeaderView::Stretch);
  nutrientsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  layout->addWidget(nutrientsTable);
}

void DetailsWidget::loadFood(int foodId, const QString &foodName) {
  currentFoodId = foodId;
  currentFoodName = foodName;
  nameLabel->setText(foodName + QString(" (ID: %1)").arg(foodId));
  addButton->setEnabled(true);

  nutrientsTable->setRowCount(0);

  std::vector<Nutrient> nutrients = repository.getFoodNutrients(foodId);

  nutrientsTable->setRowCount(static_cast<int>(nutrients.size()));
  for (int i = 0; i < static_cast<int>(nutrients.size()); ++i) {
    const auto &nut = nutrients[i];
    nutrientsTable->setItem(i, 0, new QTableWidgetItem(nut.description));
    nutrientsTable->setItem(i, 1,
                            new QTableWidgetItem(QString::number(nut.amount)));
    nutrientsTable->setItem(i, 2, new QTableWidgetItem(nut.unit));
  }
}

void DetailsWidget::onAddClicked() {
  if (currentFoodId != -1) {
    // Default 100g
    emit addToMeal(currentFoodId, currentFoodName, 100.0);
  }
}
