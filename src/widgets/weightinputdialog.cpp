#include "widgets/weightinputdialog.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

WeightInputDialog::WeightInputDialog(const QString &foodName,
                                     const std::vector<ServingWeight> &servings,
                                     QWidget *parent)
    : QDialog(parent), m_servings(servings) {
  setWindowTitle("Add to Meal - " + foodName);
  auto *layout = new QVBoxLayout(this);

  layout->addWidget(
      new QLabel("How much " + foodName + " are you adding?", this));

  auto *inputLayout = new QHBoxLayout();
  amountSpinBox = new QDoubleSpinBox(this);
  amountSpinBox->setRange(0.1, 10000.0);
  amountSpinBox->setValue(1.0);
  amountSpinBox->setDecimals(2);

  unitComboBox = new QComboBox(this);
  unitComboBox->addItem("Grams (g)", 1.0);
  unitComboBox->addItem("Ounces (oz)", GRAMS_PER_OZ);
  unitComboBox->addItem("Pounds (lb)", GRAMS_PER_LB);

  for (const auto &sw : servings) {
    unitComboBox->addItem(sw.description, sw.grams);
  }

  // Default to Grams and set value to 100 if Grams is selected
  unitComboBox->setCurrentIndex(0);
  amountSpinBox->setValue(100.0);

  // Update value when unit changes? No, let's keep it simple.
  // Usually 100g is a good default, but 1 serving might be better if available.
  if (!servings.empty()) {
    unitComboBox->setCurrentIndex(3); // First serving
    amountSpinBox->setValue(1.0);
  }

  inputLayout->addWidget(amountSpinBox);
  inputLayout->addWidget(unitComboBox);
  layout->addLayout(inputLayout);

  auto *buttonLayout = new QHBoxLayout();
  auto *okButton = new QPushButton("Add to Meal", this);
  auto *cancelButton = new QPushButton("Cancel", this);

  connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
  connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

  buttonLayout->addStretch();
  buttonLayout->addWidget(cancelButton);
  buttonLayout->addWidget(okButton);
  layout->addLayout(buttonLayout);
}

double WeightInputDialog::getGrams() const {
  double amount = amountSpinBox->value();
  double multiplier = unitComboBox->currentData().toDouble();
  return amount * multiplier;
}
