#include "widgets/searchwidget.h"
#include <QAction>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QVBoxLayout>

SearchWidget::SearchWidget(QWidget *parent) : QWidget(parent) {
  auto *layout = new QVBoxLayout(this);

  // Search bar
  auto *searchLayout = new QHBoxLayout();
  searchInput = new QLineEdit(this);
  searchInput->setPlaceholderText("Search for food...");

  searchTimer = new QTimer(this);
  searchTimer->setSingleShot(true);
  searchTimer->setInterval(600); // 600ms debounce

  connect(searchInput, &QLineEdit::textChanged, this,
          [=]() { searchTimer->start(); });
  connect(searchTimer, &QTimer::timeout, this, &SearchWidget::performSearch);
  connect(searchInput, &QLineEdit::returnPressed, this,
          &SearchWidget::performSearch);

  searchButton = new QPushButton("Search", this);
  connect(searchButton, &QPushButton::clicked, this,
          &SearchWidget::performSearch);

  searchLayout->addWidget(searchInput);
  searchLayout->addWidget(searchButton);
  layout->addLayout(searchLayout);

  // Results table
  resultsTable = new QTableWidget(this);
  resultsTable->setColumnCount(7);
  resultsTable->setHorizontalHeaderLabels(
      {"ID", "Description", "Group", "Nutr", "Amino", "Flav", "Score"});

  resultsTable->horizontalHeader()->setSectionResizeMode(1,
                                                         QHeaderView::Stretch);
  resultsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  resultsTable->setSelectionMode(QAbstractItemView::SingleSelection);
  resultsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  resultsTable->setContextMenuPolicy(Qt::CustomContextMenu);

  connect(resultsTable, &QTableWidget::cellDoubleClicked, this,
          &SearchWidget::onRowDoubleClicked);
  connect(resultsTable, &QTableWidget::customContextMenuRequested, this,
          &SearchWidget::onCustomContextMenu);

  layout->addWidget(resultsTable);
}

void SearchWidget::performSearch() {
  QString query = searchInput->text().trimmed();
  if (query.length() < 2)
    return;

  resultsTable->setRowCount(0);

  std::vector<FoodItem> results = repository.searchFoods(query);

  resultsTable->setRowCount(static_cast<int>(results.size()));
  for (int i = 0; i < static_cast<int>(results.size()); ++i) {
    const auto &item = results[i];
    resultsTable->setItem(i, 0, new QTableWidgetItem(QString::number(item.id)));
    resultsTable->setItem(i, 1, new QTableWidgetItem(item.description));
    resultsTable->setItem(
        i, 2, new QTableWidgetItem(QString::number(item.foodGroupId)));
    resultsTable->setItem(
        i, 3, new QTableWidgetItem(QString::number(item.nutrientCount)));
    resultsTable->setItem(
        i, 4, new QTableWidgetItem(QString::number(item.aminoCount)));
    resultsTable->setItem(
        i, 5, new QTableWidgetItem(QString::number(item.flavCount)));
    resultsTable->setItem(i, 6,
                          new QTableWidgetItem(QString::number(item.score)));
  }
}

void SearchWidget::onRowDoubleClicked(int row, int column) {
  Q_UNUSED(column);
  QTableWidgetItem *idItem = resultsTable->item(row, 0);
  QTableWidgetItem *descItem = resultsTable->item(row, 1);

  if (idItem != nullptr && descItem != nullptr) {
    emit foodSelected(idItem->text().toInt(), descItem->text());
  }
}

void SearchWidget::onCustomContextMenu(const QPoint &pos) {
  QTableWidgetItem *item = resultsTable->itemAt(pos);
  if (item == nullptr)
    return;

  int row = item->row();
  QTableWidgetItem *idItem = resultsTable->item(row, 0);
  QTableWidgetItem *descItem = resultsTable->item(row, 1);

  if (idItem == nullptr || descItem == nullptr)
    return;

  int foodId = idItem->text().toInt();
  QString foodName = descItem->text();

  QMenu menu(this);
  QAction *analyzeAction = menu.addAction("Analyze");
  QAction *addToMealAction = menu.addAction("Add to Meal");

  QAction *selectedAction =
      menu.exec(resultsTable->viewport()->mapToGlobal(pos));

  if (selectedAction == analyzeAction) {
    emit foodSelected(foodId, foodName);
  } else if (selectedAction == addToMealAction) {
    bool ok;
    double grams = QInputDialog::getDouble(
        this, "Add to Meal", "Amount (grams):", 100.0, 0.1, 10000.0, 1, &ok);
    if (ok) {
      emit addToMealRequested(foodId, foodName, grams);
    }
  }
}
