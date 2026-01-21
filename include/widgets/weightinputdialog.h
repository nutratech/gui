#ifndef WEIGHTINPUTDIALOG_H
#define WEIGHTINPUTDIALOG_H

#include <QComboBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QLabel>

#include "db/foodrepository.h"

class WeightInputDialog : public QDialog {
    Q_OBJECT

public:
    explicit WeightInputDialog(const QString& foodName, const std::vector<ServingWeight>& servings,
                               QWidget* parent = nullptr);

    [[nodiscard]] double getGrams() const;

private:
    QDoubleSpinBox* amountSpinBox;
    QComboBox* unitComboBox;
    std::vector<ServingWeight> m_servings;

    static constexpr double GRAMS_PER_OZ = 28.3495;
    static constexpr double GRAMS_PER_LB = 453.592;
};

#endif  // WEIGHTINPUTDIALOG_H
