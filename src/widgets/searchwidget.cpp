#include "widgets/searchwidget.h"
#include <QHBoxLayout>
#include <QHeaderView>
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
  connect(resultsTable, &QTableWidget::cellDoubleClicked, this,
          &SearchWidget::onRowDoubleClicked);

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
